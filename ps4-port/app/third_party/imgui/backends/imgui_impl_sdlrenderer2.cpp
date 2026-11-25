// BLACKBOX PS4 SDL2 renderer backend.
// Uses only SDL_RenderCopy/SDL_RenderFillRect/SDL_RenderDrawLine so it also
// builds with the older SDL2 renderer shipped by OpenOrbis.
#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_sdlrenderer2.h"
#include <SDL.h>
#include <stdint.h>
#include <algorithm>
#include <cmath>
#include <cstring>

struct ImGui_ImplSDLRenderer2_Data {
    SDL_Renderer* Renderer = nullptr;
    SDL_Texture* FontTexture = nullptr;
};
static ImGui_ImplSDLRenderer2_Data* GetData() {
    return ImGui::GetCurrentContext() ? (ImGui_ImplSDLRenderer2_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}
bool ImGui_ImplSDLRenderer2_Init(SDL_Renderer* renderer) {
    if (!renderer || !ImGui::GetCurrentContext()) return false;
    ImGuiIO& io=ImGui::GetIO();
    if (io.BackendRendererUserData) return false;
    auto* data=IM_NEW(ImGui_ImplSDLRenderer2_Data)();
    data->Renderer=renderer; io.BackendRendererUserData=data;
    io.BackendRendererName="blackbox_sdl2_compat";
    return true;
}
void ImGui_ImplSDLRenderer2_Shutdown() {
    auto* data=GetData(); if (!data) return;
    ImGui_ImplSDLRenderer2_DestroyDeviceObjects();
    ImGuiIO& io=ImGui::GetIO(); io.BackendRendererName=nullptr; io.BackendRendererUserData=nullptr;
    IM_DELETE(data);
}
void ImGui_ImplSDLRenderer2_NewFrame() {
    auto* data=GetData(); IM_ASSERT(data);
    if (!data->FontTexture) ImGui_ImplSDLRenderer2_CreateDeviceObjects();
}
static void DrawTriangle(SDL_Renderer* r, const ImDrawVert& a,const ImDrawVert& b,const ImDrawVert& c) {
    float miny=std::min(a.pos.y,std::min(b.pos.y,c.pos.y));
    float maxy=std::max(a.pos.y,std::max(b.pos.y,c.pos.y));
    for(int y=(int)std::floor(miny);y<=(int)std::ceil(maxy);++y) {
        float scan=y+0.5f, xs[3]; int n=0;
        const ImDrawVert* v[3]={&a,&b,&c};
        for(int i=0;i<3;i++){const auto& p=*v[i];const auto& q=*v[(i+1)%3];
            if((p.pos.y<=scan&&q.pos.y>scan)||(q.pos.y<=scan&&p.pos.y>scan))
                xs[n++]=p.pos.x+(scan-p.pos.y)*(q.pos.x-p.pos.x)/(q.pos.y-p.pos.y);
        }
        if(n>=2){int x1=(int)std::floor(std::min(xs[0],xs[1]));int x2=(int)std::ceil(std::max(xs[0],xs[1]));SDL_RenderDrawLine(r,x1,y,x2,y);}
    }
}
void ImGui_ImplSDLRenderer2_RenderDrawData(ImDrawData* dd) {
    auto* data=GetData(); if(!data||!dd)return;
    SDL_Renderer* r=data->Renderer;
    for(int n=0;n<dd->CmdListsCount;n++){
        const ImDrawList* list=dd->CmdLists[n];
        for(int ci=0;ci<list->CmdBuffer.Size;ci++){
            const ImDrawCmd& cmd=list->CmdBuffer[ci];
            if(cmd.UserCallback){if(cmd.UserCallback==ImDrawCallback_ResetRenderState){}else cmd.UserCallback(list,&cmd);continue;}
            SDL_Rect clip={(int)cmd.ClipRect.x,(int)cmd.ClipRect.y,
                (int)(cmd.ClipRect.z-cmd.ClipRect.x),(int)(cmd.ClipRect.w-cmd.ClipRect.y)};
            if(clip.w<=0||clip.h<=0)continue;
            SDL_RenderSetClipRect(r,&clip);
            SDL_Texture* tex=(SDL_Texture*)cmd.GetTexID();
            const ImDrawIdx* idx=list->IdxBuffer.Data+cmd.IdxOffset;
            for(unsigned int i=0;i+5<cmd.ElemCount;i+=6){
                const ImDrawVert& a=list->VtxBuffer[(int)(idx[i]+cmd.VtxOffset)];
                const ImDrawVert& b=list->VtxBuffer[(int)(idx[i+1]+cmd.VtxOffset)];
                const ImDrawVert& c=list->VtxBuffer[(int)(idx[i+2]+cmd.VtxOffset)];
                const ImDrawVert& d=list->VtxBuffer[(int)(idx[i+5]+cmd.VtxOffset)];
                bool quad=std::fabs(a.pos.y-b.pos.y)<0.1f && std::fabs(b.pos.x-c.pos.x)<0.1f &&
                    std::fabs(c.pos.y-d.pos.y)<0.1f && std::fabs(d.pos.x-a.pos.x)<0.1f;
                if(quad && tex){
                    int x=(int)std::round(std::min(a.pos.x,d.pos.x));
                    int y=(int)std::round(std::min(a.pos.y,b.pos.y));
                    int w=(int)std::round(std::fabs(b.pos.x-a.pos.x));
                    int h=(int)std::round(std::fabs(d.pos.y-a.pos.y));
                    int tw=0,th=0; SDL_QueryTexture(tex,nullptr,nullptr,&tw,&th);
                    SDL_Rect src={(int)std::round(std::min(a.uv.x,b.uv.x)*tw),
                        (int)std::round(std::min(a.uv.y,d.uv.y)*th),
                        (int)std::round(std::fabs(b.uv.x-a.uv.x)*tw),
                        (int)std::round(std::fabs(d.uv.y-a.uv.y)*th)};
                    SDL_Rect dst={x,y,w,h};
                    Uint8 cr=(Uint8)(a.col&255),cg=(Uint8)((a.col>>8)&255),
                          cb=(Uint8)((a.col>>16)&255),ca=(Uint8)((a.col>>24)&255);
                    SDL_SetTextureColorMod(tex,cr,cg,cb); SDL_SetTextureAlphaMod(tex,ca);
                    SDL_RenderCopy(r,tex,&src,&dst);
                    SDL_SetTextureColorMod(tex,255,255,255); SDL_SetTextureAlphaMod(tex,255);
                } else {
                    Uint32 col=a.col; SDL_SetRenderDrawColor(r,(Uint8)(col&255),(Uint8)((col>>8)&255),(Uint8)((col>>16)&255),(Uint8)((col>>24)&255));
                    DrawTriangle(r,a,b,c);
                    const ImDrawVert& e=list->VtxBuffer[(int)(idx[i+3]+cmd.VtxOffset)];
                    DrawTriangle(r,a,c,e);
                }
            }
        }
    }
    SDL_RenderSetClipRect(r,nullptr);
}
bool ImGui_ImplSDLRenderer2_CreateFontsTexture() {
    ImGuiIO& io=ImGui::GetIO(); auto* data=GetData(); if(!data)return false;
    unsigned char* pixels=nullptr; int w=0,h=0; io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
    data->FontTexture=SDL_CreateTexture(data->Renderer,SDL_PIXELFORMAT_ABGR8888,SDL_TEXTUREACCESS_STATIC,w,h);
    if(!data->FontTexture)return false;
    SDL_UpdateTexture(data->FontTexture,nullptr,pixels,w*4);
    SDL_SetTextureBlendMode(data->FontTexture,SDL_BLENDMODE_BLEND);
    io.Fonts->SetTexID((ImTextureID)(intptr_t)data->FontTexture);
    return true;
}
void ImGui_ImplSDLRenderer2_DestroyFontsTexture() {
    auto* data=GetData(); if(!data||!data->FontTexture)return;
    ImGui::GetIO().Fonts->SetTexID(0); SDL_DestroyTexture(data->FontTexture); data->FontTexture=nullptr;
}
bool ImGui_ImplSDLRenderer2_CreateDeviceObjects(){return ImGui_ImplSDLRenderer2_CreateFontsTexture();}
void ImGui_ImplSDLRenderer2_DestroyDeviceObjects(){ImGui_ImplSDLRenderer2_DestroyFontsTexture();}
#endif
