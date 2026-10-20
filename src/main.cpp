#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace {
constexpr int SW = 1280, SH = 720;
constexpr float PI = 3.14159265358979323846f;
enum class Screen { Menu, Racing, Paused, Results };
struct Track { const char* name; float rx, rz; Color grass, road, accent; };
const Track TRACKS[] = {
    {"COASTAL RUN", 54.0f, 38.0f, {45, 112, 78, 255}, {53, 58, 68, 255}, SKYBLUE},
    {"ALPINE PASS", 66.0f, 31.0f, {99, 119, 91, 255}, {65, 66, 70, 255}, GOLD},
    {"NEON CIRCUIT", 47.0f, 45.0f, {26, 39, 61, 255}, {35, 39, 53, 255}, MAGENTA}
};
struct Car { float angle=0, lane=0, speed=0, steer=0; Color color=RED; };
float wrap(float a) { while(a < 0) a += 2*PI; while(a >= 2*PI) a -= 2*PI; return a; }
Vector3 trackPoint(float a, const Track& t, float lane=0) {
    // A stadium circuit: long straight sections connected by rounded ends.
    const float r = std::max(8.0f, t.rz * 0.72f + lane);
    const float straightHalf = std::max(5.0f, t.rx - t.rz * 0.72f + lane);
    const float ca = std::cos(a), sa = std::sin(a);
    const float x = std::copysign(straightHalf + r * std::abs(ca), ca);
    const float z = r * sa;
    return {x, 0.0f, z};
}
float trackYaw(float a, const Track& t) {
    Vector3 p=trackPoint(a,t), q=trackPoint(a+0.002f,t);
    return std::atan2(q.x-p.x, q.z-p.z)*180.0f/PI;
}
void textCentered(const char* s, int y, int size, Color c) {
    DrawText(s, SW/2-MeasureText(s,size)/2, y, size, c);
}
void drawBox(Model cube, Vector3 pos, Vector3 scale, float yaw, Color color) {
    DrawModelEx(cube, pos, {0,1,0}, yaw, scale, color);
}
void drawCar(Model cube, const Car& car, const Track& track, bool player, float bob=0) {
    Vector3 p=trackPoint(car.angle,track,car.lane);
    p.y=0.78f+bob;
    const float yaw=trackYaw(car.angle,track);
    const float rad=yaw*PI/180.0f;
    auto part=[&](Vector3 offset, Vector3 size, Color color) {
        Vector3 pos={p.x+std::sin(rad)*offset.z+std::cos(rad)*offset.x,
                     p.y+offset.y,
                     p.z+std::cos(rad)*offset.z-std::sin(rad)*offset.x};
        drawBox(cube,pos,size,yaw,color);
    };
    Color body=car.color;
    Color glass=player?Color{80,190,220,255}:Color{85,110,135,255};
    Color dark={22,27,35,255};
    // Layered low-poly sports-car silhouette: chassis, sculpted bonnet, cabin, skirts and aero.
    part({0,0,0},{2.05f,0.55f,3.8f},body);
    part({0,0.25f,0.82f},{1.9f,0.35f,1.45f},body);
    part({0,0.52f,-0.30f},{1.48f,0.58f,1.72f},dark);
    part({0,0.68f,-0.20f},{1.30f,0.48f,1.43f},glass);
    part({0,0.28f,-1.62f},{1.72f,0.18f,0.38f},body);
    part({0,-0.12f,0},{2.12f,0.20f,3.1f},dark);
    part({0,0.03f,1.88f},{1.72f,0.12f,0.16f},Color{215,220,225,255});
    part({0,0.32f,-1.92f},{1.9f,0.12f,0.16f},body);
    // Mirrors and bright LED headlamps.
    for(float side : {-1.0f,1.0f}) {
        part({side*1.04f,0.28f,0.28f},{0.24f,0.15f,0.42f},body);
        part({side*0.69f,0.27f,1.91f},{0.38f,0.13f,0.12f},Color{255,244,185,255});
        part({side*0.69f,0.20f,-1.94f},{0.38f,0.16f,0.12f},Color{255,35,42,255});
        // Four chunky low-poly tires with inset hubs.
        for(float z : {-1.22f,1.20f}) {
            part({side*1.00f,-0.26f,z},{0.43f,0.55f,0.78f},Color{14,17,22,255});
            part({side*1.225f,-0.25f,z},{0.07f,0.30f,0.34f},Color{165,177,190,255});
            part({side*1.27f,-0.25f,z},{0.035f,0.12f,0.15f},Color{45,52,62,255});
        }
    }
    // Rear wing and racing stripe.
    part({0,0.57f,-1.66f},{1.82f,0.12f,0.18f},dark);
    part({0,0.72f,-1.66f},{1.92f,0.10f,0.28f},player?Color{245,245,245,255}:body);
    part({0,0.285f,0.65f},{0.18f,0.035f,1.65f},player?Color{245,245,245,255}:body);
}
void drawWorld(Model cube, const Track& t, float playerAngle, const Car& player, const std::vector<Car>& rivals) {
    ClearBackground({126,184,220,255});
    DrawPlane({0,-0.35f,0},{700,700},t.grass);
    // A distant low-poly horizon and broad infield keep the track grounded.
    DrawPlane({0,-0.18f,0},{180,180},t.grass);
    // Long oval circuit, segmented for a smooth curved 3D road.
    constexpr int N=320;
    for(int i=0;i<N;i++) {
        float a=2*PI*i/N, b=2*PI*(i+1)/N;
        Vector3 p=trackPoint(a,t), q=trackPoint(b,t);
        Vector3 mid={(p.x+q.x)/2,0.05f,(p.z+q.z)/2};
        float len=Vector3Distance(p,q)+0.8f;
        float yaw=std::atan2(q.x-p.x,q.z-p.z)*180/PI;
        drawBox(cube,{mid.x,-0.02f,mid.z},{19.8f,0.30f,len+0.7f},yaw,{185,65,66,255});
        drawBox(cube,mid,{18.2f,0.35f,len},yaw,t.road);
        drawBox(cube,{mid.x,0.245f,mid.z},{0.16f,0.025f,len*0.82f},yaw,{225,225,205,255});
        if(i%4==0) {
            Vector3 curbL=trackPoint(a,t,-7.9f), curbR=trackPoint(a,t,7.9f);
            drawBox(cube,{curbL.x,0.28f,curbL.z},{0.8f,0.12f,len*0.65f},yaw,(i%8==0)?RAYWHITE:RED);
            drawBox(cube,{curbR.x,0.28f,curbR.z},{0.8f,0.12f,len*0.65f},yaw,(i%8==0)?RAYWHITE:RED);
        }
        if(i%3==0) {
            Vector3 l=trackPoint(a,t,-8.1f), r=trackPoint(a,t,8.1f);
            drawBox(cube,{l.x,0.3f,l.z},{0.35f,0.06f,1.1f},yaw,(i%6==0)?RED:RAYWHITE);
            drawBox(cube,{r.x,0.3f,r.z},{0.35f,0.06f,1.1f},yaw,(i%6==0)?RED:RAYWHITE);
        }
    }
    // Start/finish stripe.
    Vector3 s=trackPoint(0,t);
    drawBox(cube,{s.x,0.29f,s.z},{16.0f,0.05f,1.3f},90,{235,235,235,255});
    // Procedural scenery: trees / rocks placed outside the track.
    for(int i=0;i<90;i++) {
        float a=2*PI*i/90.0f;
        float side=(i%2==0)?1.0f:-1.0f;
        float d=13.0f+(i%5)*2.8f;
        Vector3 p=trackPoint(a,t,side*d);
        if(t.name==std::string("NEON CIRCUIT")) {
            drawBox(cube,{p.x,1.5f,p.z},{1.0f,3.0f,1.0f},i*17.0f,t.accent);
        } else {
            drawBox(cube,{p.x,1.25f,p.z},{0.8f,2.5f,0.8f},i*23.0f,{105,73,45,255});
            drawBox(cube,{p.x,3.0f,p.z},{3.0f,3.2f,3.0f},0,(i%3==0)?Color{42,105,55,255}:Color{37,125,63,255});
        }
    }
    for(const auto& rival:rivals) drawCar(cube,rival,t,false);
    drawCar(cube,player,t,true);
}
void panel(int x,int y,int w,int h,Color c={8,12,22,220}) {
    DrawRectangle(x,y,w,h,c); DrawRectangleLines(x,y,w,h,{255,255,255,35});
}
}

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(SW,SH,"APEX RUSH | C++ 3D Racing");
    SetTargetFPS(60);
    Model cube=LoadModelFromMesh(GenMeshCube(1,1,1));
    Screen screen=Screen::Menu;
    int selectedTrack=0, menuSelection=0, laps=0, countdown=0;
    float raceTime=0, bestTime=0, lapStart=0, previousAngle=0;
    bool wasPaused=false;
    Car player; player.color={230,54,64,255};
    std::vector<Car> rivals(4);
    Color rivalColors[]={SKYBLUE, GOLD, LIME, VIOLET};
    for(int i=0;i<(int)rivals.size();++i){rivals[i].angle=0.4f+i*0.7f;rivals[i].lane=(i%2?3.0f:-3.0f);rivals[i].color=rivalColors[i];rivals[i].speed=0;}
    Camera3D camera={0};
    camera.up={0,1,0}; camera.fovy=55; camera.projection=CAMERA_PERSPECTIVE;
    while(!WindowShouldClose()) {
        float dt=std::min(GetFrameTime(),0.04f);
        const Track& track=TRACKS[selectedTrack];
        if(screen==Screen::Menu) {
            if(IsKeyPressed(KEY_UP)||IsKeyPressed(KEY_W)) menuSelection=(menuSelection+2)%3;
            if(IsKeyPressed(KEY_DOWN)||IsKeyPressed(KEY_S)) menuSelection=(menuSelection+1)%3;
            if(IsKeyPressed(KEY_LEFT)||IsKeyPressed(KEY_A)) selectedTrack=(selectedTrack+2)%3;
            if(IsKeyPressed(KEY_RIGHT)||IsKeyPressed(KEY_D)) selectedTrack=(selectedTrack+1)%3;
            if(IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_SPACE)) {
                if(menuSelection==0) {
                    player=Car{};player.color={230,54,64,255};laps=0;raceTime=0;lapStart=0;previousAngle=0;
                    for(int i=0;i<(int)rivals.size();++i){rivals[i].angle=wrap(0.4f+i*0.7f);rivals[i].lane=(i%2?3.0f:-3.0f);rivals[i].speed=0;}
                    screen=Screen::Racing;
                } else if(menuSelection==1) selectedTrack=(selectedTrack+1)%3;
                else break;
            }
        } else if(screen==Screen::Racing) {
            if(IsKeyPressed(KEY_ESCAPE)) screen=Screen::Paused;
            // WASD is consistent: W accelerates, S reverses/brakes, A/D steer left/right.
            float throttle=IsKeyDown(KEY_W)?1.0f:0.0f;
            float brake=IsKeyDown(KEY_S)?1.0f:0.0f;
            float steering=(IsKeyDown(KEY_A)?-1.0f:0.0f)+(IsKeyDown(KEY_D)?1.0f:0.0f);
            bool handbrake=IsKeyDown(KEY_SPACE);
            // Arcade vehicle dynamics: acceleration, drag, braking, grip and speed-sensitive steering.
            player.speed+=throttle*29.0f*dt;
            if(brake>0.0f) {
                if(player.speed>0.8f) player.speed-=48.0f*dt;
                else player.speed-=20.0f*dt;
            }
            player.speed-=player.speed*(handbrake?0.92f:0.11f)*dt;
            player.speed=Clamp(player.speed,-14.0f,64.0f);
            player.steer=Lerp(player.steer,steering,Clamp(dt*(handbrake?4.0f:8.5f),0,1));
            const float speed01=Clamp(std::abs(player.speed)/64.0f,0.0f,1.0f);
            const float grip=handbrake?0.42f:1.0f;
            // Steering changes heading as well as line; lateral grip naturally fades in a handbrake slide.
            player.angle=wrap(player.angle + player.steer*player.speed*dt/std::sqrt(track.rx*track.rz)*0.82f*grip);
            player.lane+=player.steer*(handbrake?12.0f:3.6f)*speed01*dt;
            // The car is not rail-locked: leaving the asphalt is possible but costs grip and speed.
            const float roadEdge=9.0f;
            const float offroad=std::max(0.0f,std::abs(player.lane)-roadEdge);
            if(offroad>0.0f) {
                player.speed*=std::max(0.0f,1.0f-0.85f*dt);
                player.steer=Lerp(player.steer,0.0f,dt*0.45f);
            }
            float radius=std::sqrt(track.rx*track.rz);
            float old=player.angle;
            // Reverse travel follows the path backwards; no invisible lane snapping.
            player.angle=wrap(player.angle+player.speed/radius*dt*0.55f);
            if(old>5.4f&&player.angle<0.8f&&player.speed>3.0f){laps++;if(laps>0){float lap=raceTime-lapStart;if(lapStart>0&&(bestTime==0||lap<bestTime))bestTime=lap;lapStart=raceTime;}if(laps>=3)screen=Screen::Results;}
            raceTime+=dt;
            for(auto& ai:rivals) {
                float target=25.0f+(std::sin(ai.angle*3.0f)*3.0f);
                ai.speed=Lerp(ai.speed,target,dt*0.65f);
                ai.angle=wrap(ai.angle+ai.speed/radius*dt*0.55f);
            }
            // Soft collision response when cars occupy nearby track positions.
            for(auto& ai:rivals) {
                float da=std::abs(std::remainder(player.angle-ai.angle,2*PI));
                if(da<0.055f&&std::abs(player.lane-ai.lane)<2.8f) {
                    // Contact slows both cars and pushes them apart laterally.
                    player.speed*=0.72f;
                    ai.speed*=0.86f;
                    float side=player.lane<=ai.lane?-1.0f:1.0f;
                    player.lane+=side*1.8f;
                    ai.lane-=side*0.9f;
                }
            }
            // Solid scenery collision: tree trunks and trackside posts are obstacles.
            for(int i=0;i<90;i++) {
                float a=2*PI*i/90.0f;
                float side=(i%2==0)?1.0f:-1.0f;
                float d=13.0f+(i%5)*2.8f;
                Vector3 obstacle=trackPoint(a,track,side*d);
                Vector3 pp=trackPoint(player.angle,track,player.lane);
                if(Vector3Distance({pp.x,0,pp.z},obstacle)<2.2f) {
                    player.speed*=0.45f;
                    player.lane+=player.lane>=0.0f?-1.4f:1.4f;
                }
            }
        } else if(screen==Screen::Paused) {
            if(IsKeyPressed(KEY_ESCAPE)||IsKeyPressed(KEY_ENTER)) screen=Screen::Racing;
            if(IsKeyPressed(KEY_M)) screen=Screen::Menu;
        } else if(screen==Screen::Results) {
            if(IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_ESCAPE)) screen=Screen::Menu;
        }
        // Follow camera looks toward the next section of track.
        Vector3 carPos=trackPoint(player.angle,track,player.lane);
        Vector3 ahead=trackPoint(player.angle+0.16f,track);
        float yaw=trackYaw(player.angle,track)*PI/180.0f;
        camera.target={carPos.x,1.0f,carPos.z};
        camera.position={carPos.x-std::sin(yaw)*13.5f,7.2f,carPos.z-std::cos(yaw)*13.5f};
        camera.target=Vector3Lerp(camera.target,{ahead.x,0.5f,ahead.z},0.24f);
        BeginDrawing();
        if(screen==Screen::Menu) {
            ClearBackground({9,15,27,255});
            // Animated 3D showroom backdrop.
            Camera3D showroom={{0,12,105},{0,0,0},{0,1,0},42,CAMERA_PERSPECTIVE};
            BeginMode3D(showroom);
            DrawPlane({0,-2,0},{220,220},{19,29,45,255});
            for(int i=-8;i<=8;i++) DrawCube({(float)i*9,-1.8f,0},0.08f,0.08f,180,{35,55,78,255});
            Car display;display.angle=(float)GetTime()*0.35f;display.color={235,49,65,255};
            drawBox(cube,{0,0,0},{4.0f,1.3f,7.0f},(float)GetTime()*10.0f,display.color);
            drawBox(cube,{0,0.8f,-0.3f},{2.7f,0.8f,3.0f},(float)GetTime()*10.0f,{175,220,245,255});
            EndMode3D();
            DrawRectangleGradientV(0,0,SW,SH,{4,8,17,105},{4,8,17,245});
            DrawText("APEX",78,62,90,RAYWHITE); DrawText("RUSH",78,145,90,{236,55,70,255});
            DrawText("3D ARCADE RACING",84,249,20,{140,178,205,255});
            DrawRectangle(84,290,64,4,track.accent);
            const char* opts[]={"START RACE","SELECT TRACK","QUIT"};
            for(int i=0;i<3;i++) {
                int y=340+i*57;
                if(i==menuSelection) {DrawRectangle(76,y-7,340,45,{235,55,70,220});DrawText(">",94,y+2,25,RAYWHITE);}
                DrawText(opts[i],132,y+2,23,(i==menuSelection)?RAYWHITE:Color{165,181,202,255});
            }
            panel(790,430,400,175,{8,14,25,225});
            DrawText("CURRENT CIRCUIT",818,452,16,{135,159,185,255});
            DrawText(track.name,818,480,27,RAYWHITE);
            DrawText("A / D  CHANGE TRACK",818,523,16,track.accent);
            DrawText("WASD  DRIVE",818,552,15,{150,169,193,255});
            DrawText("ENTER  CONFIRM",818,576,15,{150,169,193,255});
            DrawText("A R C A D E   R A C I N G",84,660,14,{89,112,140,255});
        } else {
            BeginMode3D(camera);
            drawWorld(cube,track,player.angle,player,rivals);
            EndMode3D();
            // Speedometer and race HUD.
            panel(28,25,270,104);DrawText("SPEED",48,43,15,{155,175,201,255});
            DrawText(TextFormat("%03d", (int)(std::abs(player.speed)*4.1f)),45,62,42,RAYWHITE);
            DrawText("KM/H",145,84,13,track.accent);
            DrawRectangle(48,112,205,7,{35,43,57,255});
            DrawRectangle(48,112,(int)(205*Clamp(std::abs(player.speed)/58.0f,0,1)),7,track.accent);
            panel(SW-280,25,250,105);DrawText(track.name,SW-258,43,18,RAYWHITE);
            DrawText(TextFormat("LAP  %d / 3",std::min(laps+1,3)),SW-258,70,22,track.accent);
            DrawText(TextFormat("TIME  %02d:%05.2f",(int)(raceTime/60),std::fmod(raceTime,60.0)),SW-258,98,16,RAYWHITE);
            DrawText("W ACCELERATE  S BRAKE/REVERSE  A/D STEER  SPACE HANDBRAKE  ESC PAUSE",SW/2-300,SH-33,14,RAYWHITE);
            if(screen==Screen::Paused) {
                DrawRectangle(0,0,SW,SH,{4,7,15,190});panel(SW/2-220,SH/2-130,440,260);
                textCentered("RACE PAUSED",SH/2-88,36,RAYWHITE);
                textCentered("ENTER / ESC  RESUME",SH/2-20,19,track.accent);
                textCentered("M  MAIN MENU",SH/2+22,19,{180,195,215,255});
            } else if(screen==Screen::Results) {
                DrawRectangle(0,0,SW,SH,{4,7,15,200});panel(SW/2-260,SH/2-150,520,300);
                textCentered("FINISH!",SH/2-110,48,{245,205,83,255});
                textCentered("YOU COMPLETED 3 LAPS",SH/2-43,22,RAYWHITE);
                textCentered(TextFormat("TOTAL TIME  %02d:%05.2f",(int)(raceTime/60),std::fmod(raceTime,60.0)),SH/2+2,21,track.accent);
                textCentered("PRESS ENTER TO CONTINUE",SH/2+82,17,{170,190,215,255});
            }
        }
        EndDrawing();
    }
    UnloadModel(cube);
    CloseWindow();
    return 0;
}
