#if INTERFACE

#include <Engine/Includes/Standard.h>

class Discord {
public:
    static bool Initialized;
};
#endif

#include <Engine/Extensions/Discord.h>
#include <Engine/Application.h>

#ifdef WIN32
// #pragma pack(push, 8)
// #include <discord_game_sdk.h>
// #pragma pack(pop)

#include <discord_rpc.h>

#define HAS_DISCORD_RPC

void* library = NULL;
void (*_Discord_Initialize)(const char*, DiscordEventHandlers*, int, const char*) = NULL;
void (*_Discord_UpdatePresence)(const DiscordRichPresence*) = NULL;
void (*_Discord_ClearPresence)(void) = NULL;
void (*_Discord_Shutdown)(void) = NULL;

#endif

bool Discord::Initialized = false;

// struct DiscordUser {
//     char* username;
//     char* discriminator;
//     char* userId;
// };
// void Discord_Ready(const DiscordUser* connectedUser) { Discord::Initialized = true; }
// void Discord_Disconnected(int errcode, const char* message) { }
// void Discord_Error(int errcode, const char* message) { }
// void Discord_Spectate(const char* secret) { }

// Discord::Init("623985235150766081", "");

PUBLIC STATIC void Discord::Init(const char* application_id, const char* steam_id) {
    #ifdef HAS_DISCORD_RPC

    library = SDL_LoadObject("discord-rpc.dll");
    if (!library)
        return;

    _Discord_Initialize = (void (*)(const char*, DiscordEventHandlers*, int, const char*))SDL_LoadFunction(library, "Discord_Initialize");
    _Discord_UpdatePresence = (void (*)(const DiscordRichPresence*))SDL_LoadFunction(library, "Discord_UpdatePresence");
    _Discord_ClearPresence = (void (*)(void))SDL_LoadFunction(library, "Discord_ClearPresence");
    _Discord_Shutdown = (void (*)(void))SDL_LoadFunction(library, "Discord_Shutdown");

    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));

    // handlers.ready = Discord_Ready;
    // handlers.errored = Discord_Error;
    // handlers.disconnected = Discord_Disconnected;

    _Discord_Initialize(application_id, &handlers, 1, steam_id);

    // struct Application {
    //     struct IDiscordCore* core;
    //     struct IDiscordUsers* users;
    // };
    //
    // struct Application app;
    // // Don't forget to memset or otherwise initialize your classes!
    // memset(&app, 0, sizeof(app));
    //
    // struct IDiscordCoreEvents events;
    // memset(&events, 0, sizeof(events));
    //
    // struct DiscordCreateParams params;
    // params.client_id = 379010971139702784;
    // params.flags = DiscordCreateFlags_Default;
    // params.events = &events;
    // params.event_data = &app;
    //
    // DiscordCreate(DISCORD_VERSION, &params, &app.core);
    Discord::Initialized = true;
    Discord::UpdatePresence(NULL);
    return;
    #endif

    Discord::Initialized = false;
}

PUBLIC STATIC void Discord::UpdatePresence(char* details) {
    Discord::UpdatePresence(details, NULL);
}
PUBLIC STATIC void Discord::UpdatePresence(char* details, char* state) {
    Discord::UpdatePresence(details, state, NULL);
}
PUBLIC STATIC void Discord::UpdatePresence(char* details, char* state, char* image_key) {
    Discord::UpdatePresence(details, state, image_key, 0, 0);
}
PUBLIC STATIC void Discord::UpdatePresence(char* details, char* state, char* image_key, time_t start_time) {
    Discord::UpdatePresence(details, state, image_key, 0, 0, start_time);
}
PUBLIC STATIC void Discord::UpdatePresence(char* details, char* state, char* image_key, int party_size, int party_max) {
    Discord::UpdatePresence(details, state, image_key, party_size, party_max, 0);
}
PUBLIC STATIC void Discord::UpdatePresence(char* details, char* state, char* image_key, int party_size, int party_max, time_t start_time) {
    if (!Discord::Initialized) return;

    #ifdef HAS_DISCORD_RPC
    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));

    discordPresence.details = details;
    discordPresence.state = state;
    discordPresence.largeImageKey = image_key;
    //discordPresence.partyId = 0;
    discordPresence.partySize = party_size;
    discordPresence.partyMax = party_max;
    discordPresence.startTimestamp = start_time;
    discordPresence.instance = 1;
    _Discord_UpdatePresence(&discordPresence);
    #endif
}

PUBLIC STATIC void Discord::Dispose() {
    if (!Discord::Initialized) return;

    #ifdef HAS_DISCORD_RPC
    _Discord_Shutdown();
    SDL_UnloadObject(library);
    #endif
}
