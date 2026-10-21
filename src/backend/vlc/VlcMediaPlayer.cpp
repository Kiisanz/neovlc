#include "VlcMediaPlayer.h"

#include "VlcContext.h"

#include <vlc/vlc.h>

namespace neovlc::backend {

VlcMediaPlayer::VlcMediaPlayer(VlcContext &context, QObject *parent)
    : QObject(parent), m_context(nullptr)
{
    if (!context.isAvailable()) {
        m_errorString = context.errorString().isEmpty()
            ? QStringLiteral("Konteks libVLC tidak tersedia; pemutar media tidak dapat dibuat.")
            : context.errorString();
        return;
    }

    m_player.reset(libvlc_media_player_new(context.instance()));
    if (!m_player) {
        const char *error = libvlc_errmsg();
        m_errorString = error
            ? QString::fromUtf8(error)
            : QStringLiteral("Gagal membuat pemutar media libVLC.");
        return;
    }

    // Only track the context once registration establishes the invariant that
    // every registered player can be detached before the context is destroyed.
    context.registerPlayer(this);
    m_context = &context;
}

VlcMediaPlayer::~VlcMediaPlayer()
{
    if (m_context)
        m_context->unregisterPlayer(this);
    if (m_player)
        libvlc_media_player_stop(m_player.get());
}

void VlcMediaPlayer::PlayerDeleter::operator()(libvlc_media_player_t *player) const
{
    libvlc_media_player_release(player);
}

} // namespace neovlc::backend
