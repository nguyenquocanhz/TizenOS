#include "tizen/player.h"
#include <stdlib.h>
#include <stdio.h>

struct _TizenPlayer {
    TizenPlayerState state;
    char *uri;
};

// Extern các hàm cho display và audio sink
extern void player_display_init(void);
extern void player_audio_init(void);

TizenPlayer* tizen_player_create(void) {
    TizenPlayer *player = malloc(sizeof(TizenPlayer));
    player->state = PLAYER_STATE_IDLE;
    player->uri = NULL;
    
    // Khởi tạo Wayland subsurface sink cho video
    player_display_init();
    
    // Khởi tạo PipeWire audio sink cho âm thanh
    player_audio_init();
    
    return player;
}

void tizen_player_destroy(TizenPlayer *player) {
    if (player) {
        free(player->uri);
        free(player);
    }
}

void tizen_player_set_uri(TizenPlayer *player, const char *uri) {
    if (player) {
        player->uri = uri ? strdup(uri) : NULL;
        player->state = PLAYER_STATE_READY;
        printf("Đã thiết lập URI: %s\n", uri);
    }
}

void tizen_player_play(TizenPlayer *player) {
    if (player && (player->state == PLAYER_STATE_READY || player->state == PLAYER_STATE_PAUSED)) {
        player->state = PLAYER_STATE_PLAYING;
        printf("Trạng thái: Đang phát (Playing)\n");
    }
}

void tizen_player_pause(TizenPlayer *player) {
    if (player && player->state == PLAYER_STATE_PLAYING) {
        player->state = PLAYER_STATE_PAUSED;
        printf("Trạng thái: Tạm dừng (Paused)\n");
    }
}

void tizen_player_stop(TizenPlayer *player) {
    if (player) {
        player->state = PLAYER_STATE_STOPPED;
        printf("Trạng thái: Dừng (Stopped)\n");
    }
}

TizenPlayerState tizen_player_get_state(TizenPlayer *player) {
    return player ? player->state : PLAYER_STATE_IDLE;
}
