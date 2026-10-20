#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace {
constexpr int W=1440,H=900;
constexpr float PI=3.14159265f;
enum class Mode { Menu, Drive, Paused };
struct Node { Vector3 p, prev; float invMass=1; };
struct Spring { int a,b; float rest, stiffness; bool structural=false; };
struct Debris { Vector3 p,v,scale; Color color; float life=20; };
struct Car {
 std::vector<Node> nodes; std::vector<Spring> springs;
 Vector3 pos{0,1.4f,0}, velocity{0,0,0}; float yaw=0, steer=0, throttle=0;
 float health=1, speed=0; int broken=0; Color paint{42,151,231,255};
};
struct Obstacle { Vector3 p,size; Color c; };
float clampf(float v,float a,float b){return std::max(a,std::min(b,v));}
Vector3 localToWorld(Vector3 p,float yaw,Vector3 origin){
 float s=std::sin(yaw),c=std::cos(yaw);
 return {origin.x+p.x*c+p.z*s,origin.y+p.y,origin.z-p.x*s+p.z*c};
}
Vector3 worldToLocal(Vector3 p,float yaw,Vector3 origin){
 p=Vector3Subtract(p,origin);float s=std::sin(yaw),c=std::cos(yaw);
 return {p.x*c-p.z*s,p.y,p.x*s+p.z*c};
}
void addNode(Car& c,Vector3 local,float mass=1){
 Vector3 p=localToWorld(local,c.yaw,c.pos);c.nodes.push_back({p,p,mass>0?1.0f/mass:0.0f});
}
void addSpring(Car& c,int a,int b,float stiffness=0.88f,bool structural=false){
 c.springs.push_back({a,b,Vector3Distance(c.nodes[a].p,c.nodes[b].p),stiffness,structural});
}
void buildCar(Car& c){
 c.nodes.clear();c.springs.clear();
 // A deformable space frame: lower rails, upper cabin rails, front/rear crumple zones.
 const Vector3 pts[]={
 {-1.0f,.1f,-2.15f},{1.0f,.1f,-2.15f},{-1.0f,.1f,2.15f},{1.0f,.1f,2.15f},
 {-1.05f,.65f,-1.45f},{1.05f,.65f,-1.45f},{-1.0f,.65f,1.35f},{1.0f,.65f,1.35f},
 {-.82f,1.35f,-.75f},{.82f,1.35f,-.75f},{-.82f,1.35f,.75f},{.82f,1.35f,.75f},
 {-.98f,.48f,-2.45f},{.98f,.48f,-2.45f},{-.98f,.48f,2.45f},{.98f,.48f,2.45f}};
 for(auto p:pts)addNode(c,p);
 const int edges[][2]={{0,1},{0,2},{1,3},{2,3},{0,4},{1,5},{2,6},{3,7},{4,5},{6,7},{4,6},{5,7},{4,8},{5,9},{8,9},{8,10},{9,11},{10,11},{6,10},{7,11},{0,12},{1,13},{2,14},{3,15},{12,13},{14,15},{4,12},{5,13},{6,14},{7,15},{8,11},{9,10},{0,5},{1,4},{2,7},{3,6},{4,10},{5,11},{6,9},{7,8}};
 for(auto &e:edges)addSpring(c,e[0],e[1],(e[0]>=12||e[1]>=12)?0.56f:0.9f,true);
}
void resetCar(Car& c){c=Car{};c.paint={42,151,231,255};buildCar(c);}
void box(Vector3 p,Vector3 size,Color col){DrawCubeV(p,size,col);}
void drawVehicle(const Car& c){
 // Body panels are separate meshes; their attachment points follow the deformable frame.
 auto P=[&](Vector3 v){return localToWorld(v,c.yaw,c.pos);};
 Vector3 a=P({0,.25f,-.05f});
 float frontDamage=clampf((2.2f-c.nodes[12].p.y)*.25f,0,.35f);
 box(P({0,.28f,-.05f}),{1.95f, .45f-frontDamage,3.85f},c.paint);
 box(P({0,.78f,.15f}),{1.55f,.7f,1.75f},{42,58,73,255});
 box(P({0,1.12f,.15f}),{1.42f,.08f,1.65f},{105,174,205,255});
 box(P({0,.7f,-.95f}),{1.45f,.38f,.62f},c.paint);
 box(P({0,.68f,1.25f}),{1.6f,.32f,.75f},c.paint);
 for(int side:{-1,1})for(int axle:{-1,1}){
  Vector3 wheel=P({side*1.03f,.02f,axle*1.38f});
  DrawCylinderEx({wheel.x,wheel.y-.18f,wheel.z},{wheel.x,wheel.y+.18f,wheel.z},.39f,.39f,12,{24,26,31,255});
  DrawCylinderEx({wheel.x,wheel.y-.19f,wheel.z},{wheel.x,wheel.y+.19f,wheel.z},.21f,.21f,12,{150,160,171,255});
 }
 box(P({-.65f,.39f,-2.0f}),{.38f,.2f,.12f},YELLOW);
 box(P({.65f,.39f,-2.0f}),{.38f,.2f,.12f},YELLOW);
 box(P({-.65f,.4f,1.99f}),{.38f,.2f,.12f},RED);
 box(P({.65f,.4f,1.99f}),{.38f,.2f,.12f},RED);
 // Chassis rails reveal deformation after a crash.
 for(const auto& s:c.springs)if(s.structural&&s.a<12&&s.b<12){
  Vector3 x=c.nodes[s.a].p,y=c.nodes[s.b].p;
  Color col=Vector3Distance(x,y)>s.rest*1.35f?RED:Fade({65,76,88,255},.72f);
  DrawLine3D(x,y,col);
 }
}
void solve(Car& c,float dt,std::vector<Debris>& debris){
 // Verlet integration plus iterative distance constraints. Weak crumple-zone links can permanently yield.
 for(auto& n:c.nodes){
  if(n.invMass==0)continue;
  Vector3 v=Vector3Scale(Vector3Subtract(n.p,n.prev),.992f);
  n.prev=n.p;n.p=Vector3Add(n.p,Vector3Add(v,{0,-9.81f*dt*dt,0}));
  if(n.p.y<.38f){n.p.y=.38f;if(n.prev.y-n.p.y>0.18f)n.prev.y=n.p.y;}
 }
 for(int iter=0;iter<7;iter++)for(auto& s:c.springs){
  auto& a=c.nodes[s.a];auto& b=c.nodes[s.b];
  Vector3 d=Vector3Subtract(b.p,a.p);float len=Vector3Length(d);if(len<.0001f)continue;
  float strain=std::abs(len-s.rest)/s.rest;
  if(s.structural&&strain>.62f&&s.stiffness>.3f){
   s.stiffness=std::max(.12f,s.stiffness-.018f);
   if(c.broken<99)c.broken++;
   c.health=std::max(.08f,c.health-.0008f);
   if(strain>.95f&&s.stiffness<.35f)s.structural=false;
  }
  float w=a.invMass+b.invMass;if(w<=0)continue;
  Vector3 correction=Vector3Scale(d,((len-s.rest)/len)*s.stiffness);
  a.p=Vector3Add(a.p,Vector3Scale(correction,a.invMass/w));
  b.p=Vector3Subtract(b.p,Vector3Scale(correction,b.invMass/w));
 }
 // Chassis centre follows its simulated frame while retaining player control.
 Vector3 avg={0};for(int i=0;i<12;i++)avg=Vector3Add(avg,c.nodes[i].p);
 c.pos=Vector3Lerp(c.pos,Vector3Scale(avg,1.0f/12.0f),.32f);
}
void hit(Car& c,Vector3 point,Vector3 normal,float impact,std::vector<Debris>& debris){
 if(impact<2.8f)return;
 c.health=std::max(0.0f,c.health-impact*.003f);
 c.velocity=Vector3Add(c.velocity,Vector3Scale(normal,impact*.22f));
 for(auto& n:c.nodes){
  float d=Vector3Distance(n.p,point);
  if(d<2.8f){float weight=(1-d/2.8f)*std::min(impact*0.035f,1.8f);
   n.p=Vector3Add(n.p,Vector3Scale(normal,weight));
   n.prev=Vector3Subtract(n.prev,Vector3Scale(normal,weight*.3f));
  }
 }
 if(impact>8&&debris.size()<180)for(int i=0;i<3;i++)
  debris.push_back({point,{(float)GetRandomValue(-7,7),GetRandomValue(2,9),(float)GetRandomValue(-7,7)},{.15f,.12f,.35f},c.paint,8});
}
}
int main(){
 SetConfigFlags(FLAG_MSAA_4X_HINT|FLAG_VSYNC_HINT);InitWindow(W,H,"FLEX DRIVE | Vehicle Crash Lab");SetTargetFPS(60);
 Mode mode=Mode::Menu;Car car;resetCar(car);std::vector<Debris> debris;
 std::vector<Obstacle> obstacles={
 {{0,1,-35},{18,2,2},{150,155,164,255}},{{-19,1,-15},{2,2,20},{150,155,164,255}},
 {{19,1,-15},{2,2,20},{150,155,164,255}},{{0,1,12},{28,2,2},{150,155,164,255}},
 {{-9,1,28},{2,2,15},{150,155,164,255}},{{12,1,32},{18,2,2},{150,155,164,255}},
 {{-28,1,0},{4,2,4},{226,112,45,255}},{{26,1,17},{4,2,4},{226,112,45,255}},
 {{0,1,45},{12,2,3},{225,183,63,255}},{{-8,2,5},{8,4,2},{90,105,120,255}}
 };
 Camera3D cam={{0,7,13},{0,1,0},{0,1,0},60,CAMERA_PERSPECTIVE};
 bool showHelp=true;float damageFlash=0;
 while(!WindowShouldClose()){
  float dt=std::min(GetFrameTime(),.025f);
  if(mode==Mode::Menu){
   if(IsKeyPressed(KEY_ENTER)){resetCar(car);debris.clear();mode=Mode::Drive;}
   if(IsKeyPressed(KEY_ESCAPE))break;
  }else if(mode==Mode::Drive){
   if(IsKeyPressed(KEY_ESCAPE))mode=Mode::Paused;
   float gas=IsKeyDown(KEY_W)||IsKeyDown(KEY_UP)?1:0;
   float brake=IsKeyDown(KEY_S)||IsKeyDown(KEY_DOWN)?1:0;
   float turn=(IsKeyDown(KEY_A)||IsKeyDown(KEY_LEFT)?-1.0f:0)+(IsKeyDown(KEY_D)||IsKeyDown(KEY_RIGHT)?1.0f:0);
   car.steer=Lerp(car.steer,turn,dt*5);
   float forward=car.velocity.x*std::sin(car.yaw)+car.velocity.z*std::cos(car.yaw);
   forward+=gas*16*dt-brake*23*dt-forward*.34f*dt;
   forward=clampf(forward,-12,42);
   car.yaw+=car.steer*dt*(.45f+std::abs(forward)*.025f)*(forward<0?-1:1);
   Vector3 desired={std::sin(car.yaw)*forward,car.velocity.y,std::cos(car.yaw)*forward};
   car.velocity.x=Lerp(car.velocity.x,desired.x,dt*2.2f);
   car.velocity.z=Lerp(car.velocity.z,desired.z,dt*2.2f);
   car.pos=Vector3Add(car.pos,Vector3Scale(car.velocity,dt));
   for(auto& n:car.nodes)n.p=Vector3Add(n.p,Vector3Scale(car.velocity,dt));
   // Concrete walls, barriers and test props collide with the body and deform it.
   for(auto& o:obstacles){
    Vector3 local=worldToLocal(car.pos,0,o.p);
    if(std::abs(local.x)<o.size.x*.5f+1.05f&&std::abs(local.z)<o.size.z*.5f+2.0f&&car.pos.y<o.size.y+2){
     Vector3 normal=Vector3Normalize({car.pos.x-o.p.x,0,car.pos.z-o.p.z});
     if(Vector3Length(normal)<.1f)normal={0,0,1};
     float impact=Vector3Length(car.velocity);
     hit(car,o.p,normal,impact,debris);car.velocity=Vector3Scale(car.velocity,-.32f);
     car.pos=Vector3Add(car.pos,Vector3Scale(normal,.7f));damageFlash=.18f;
    }
   }
   solve(car,dt,debris);
   if(car.pos.y<.5f){car.pos.y=.5f;car.velocity.y=0;}
   for(auto& d:debris){d.life-=dt;d.v.y-=9.8f*dt;d.p=Vector3Add(d.p,Vector3Scale(d.v,dt));if(d.p.y<.1f){d.p.y=.1f;d.v.y*=-.25f;}}
   debris.erase(std::remove_if(debris.begin(),debris.end(),[](auto& d){return d.life<=0;}),debris.end());
   damageFlash=std::max(0.0f,damageFlash-dt);
   if(IsKeyPressed(KEY_R)){resetCar(car);debris.clear();}
   if(IsKeyPressed(KEY_TAB))showHelp=!showHelp;
  }else if(mode==Mode::Paused){
   if(IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_ESCAPE))mode=Mode::Drive;
   if(IsKeyPressed(KEY_M))mode=Mode::Menu;
   if(IsKeyPressed(KEY_R)){resetCar(car);debris.clear();mode=Mode::Drive;}
  }
  float sy=std::sin(car.yaw),cy=std::cos(car.yaw);
  Vector3 look=Vector3Add(car.pos,{sy*2,1.2f,cy*2});
  Vector3 targetCam=Vector3Add(car.pos,{-sy*11,5.5f,-cy*11});
  cam.target=Vector3Lerp(cam.target,look,dt*4);cam.position=Vector3Lerp(cam.position,targetCam,dt*3);
  BeginDrawing();ClearBackground({130,174,202,255});
  if(mode==Mode::Menu){
   Camera3D intro={{13,9,16},{0,1,0},{0,1,0},48,CAMERA_PERSPECTIVE};BeginMode3D(intro);
   DrawPlane({0,-.1f,0},{200,200},{48,69,76,255});
   for(int i=-5;i<=5;i++){DrawLine3D({(float)i*5,0,-70},{(float)i*5,0,70},{73,96,103,255});DrawLine3D({-70,0,(float)i*5},{70,0,(float)i*5},{73,96,103,255});}
   drawVehicle(car);EndMode3D();
   DrawRectangleGradientV(0,0,W,H,{4,9,17,175},{4,9,17,245});
   DrawText("FLEX",86,78,94,RAYWHITE);DrawText("DRIVE",86,166,94,{65,177,244,255});
   DrawText("VEHICLE PHYSICS  /  CRASH LAB",94,277,21,{174,197,215,255});
   DrawRectangle(94,324,76,5,{65,177,244,255});
   DrawText("OPEN WORLD TEST AREA",94,370,24,RAYWHITE);
   DrawText("DEFORMABLE CHASSIS  -  COLLISION DAMAGE",94,410,17,{151,174,196,255});
   DrawText("SPRING-BASED STRUCTURAL SIMULATION",94,440,17,{151,174,196,255});
   DrawRectangle(88,515,330,64,{35,132,201,245});DrawText("ENTER  |  START DRIVING",112,535,20,RAYWHITE);
   DrawText("WASD to drive  |  R to reset  |  ESC to pause",94,620,17,{174,197,215,255});
   DrawText("PROTOTYPE BUILD 0.2",94,842,14,{106,132,155,255});
  }else{
   BeginMode3D(cam);
   DrawPlane({0,-.1f,0},{300,300},{72,104,77,255});
   for(int x=-12;x<=12;x++)for(int z=-12;z<=12;z++)DrawCube({x*10.0f,-.025f,z*10.0f},.025f,.025f,.025f,{130,153,117,255});
   // Asphalt test pad, skid lanes, ramps and obstacles.
   DrawCube({0,.03f,0},70,.08f,110,{81,86,88,255});
   for(int i=-3;i<=3;i++)DrawCube({i*8,.09f,0},.12f,.025f,96,{220,215,174,255});
   DrawCube({-16,.45f,28},9,.8f,12,{107,118,126,255});
   DrawCube({18,.35f,-28},12,.6f,10,{107,118,126,255});
   for(auto&o:obstacles)box(o.p,o.size,o.c);
   // Crash dummies and pylons for impact tests.
   for(int i=0;i<10;i++){float x=-32+(i%5)*16.0f,z=38+(i/5)*8.0f;DrawCylinder({x,.65f,z},.45f,.35f,1.3f,10,ORANGE);}
   drawVehicle(car);
   for(auto& d:debris){box(d.p,d.scale,Fade(d.color,clampf(d.life/3,0,1)));}
   EndMode3D();
   DrawRectangle(0,0,W,84,{7,13,21,220});
   DrawText("FLEX DRIVE",25,18,27,{65,177,244,255});
   DrawText("TEST MAP  /  VEHICLE 01",25,51,14,{161,181,198,255});
   DrawText(TextFormat("SPEED  %03d KM/H",(int)(Vector3Length(car.velocity)*3.6f)),430,20,25,RAYWHITE);
   DrawText(TextFormat("STRUCTURE  %d LINKS DAMAGED",car.broken),430,52,15,car.broken?ORANGE:LIGHTGRAY);
   DrawText("VEHICLE CONDITION",W-280,19,15,RAYWHITE);
   DrawRectangle(W-280,45,240,16,{55,61,70,255});DrawRectangle(W-280,45,(int)(240*car.health),16,car.health>.65f?LIME:(car.health>.3f?ORANGE:RED));
   if(showHelp){DrawRectangle(20,H-120,440,91,{6,13,20,215});DrawText("WASD / ARROWS  DRIVE & STEER",36,H-105,16,RAYWHITE);DrawText("R  RESET VEHICLE     TAB  HIDE HELP",36,H-78,16,RAYWHITE);DrawText("ESC  PAUSE",36,H-52,16,{110,190,235,255});}
   if(damageFlash>0)DrawRectangleLinesEx({0,0,(float)W,(float)H},7,Fade(RED,damageFlash*3));
   if(mode==Mode::Paused){DrawRectangle(0,0,W,H,{0,0,0,190});DrawText("PAUSED",W/2-105,H/2-100,48,RAYWHITE);DrawText("ENTER / ESC  RESUME",W/2-145,H/2-20,20,{65,177,244,255});DrawText("R  RESET VEHICLE",W/2-115,H/2+20,18,RAYWHITE);DrawText("M  MAIN MENU",W/2-92,H/2+55,18,RAYWHITE);}
  }
  EndDrawing();
 }
 CloseWindow();return 0;
}