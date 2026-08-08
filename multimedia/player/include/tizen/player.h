#ifndef TIZEN_PLAYER_H
#define TIZEN_PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PLAYER_STATE_IDLE,
    PLAYER_STATE_READY,
    PLAYER_STATE_PLAYING,
    PLAYER_STATE_PAUSED,
    PLAYER_STATE_STOPPED
} TizenPlayerState;

typedef struct _TizenPlayer TizenPlayer;

TizenPlayer* tizen_player_create(void);
void tizen_player_destroy(TizenPlayer *player);

void tizen_player_set_uri(TizenPlayer *player, const char *uri);
void tizen_player_play(TizenPlayer *player);
void tizen_player_pause(TizenPlayer *player);
void tizen_player_stop(TizenPlayer *player);
TizenPlayerState tizen_player_get_state(TizenPlayer *player);

#ifdef __cplusplus
}
#endif

#endif // TIZEN_PLAYER_H
