#include "VlcContext.h"

#include "VlcMediaPlayer.h"

#include <utility>
#include <vlc/vlc.h>

namespace neovlc::backend {

VlcContext::VlcContext(QObject *parent)
    : QObject(parent), m_instance(libvlc_new(0, nullptr))
{
    if (!m_instance) {
        const char *error = libvlc_errmsg();
        m_errorString = error
            ? QString::fromUtf8(error)
            : QStringLiteral("Gagal menginisialisasi libVLC. Periksa plugin VLC.");
    }
}

VlcContext::~VlcContext()
{
    Q_ASSERT_X(m_players.isEmpty(), "VlcContext::~VlcContext",
               "Destroy all VlcMediaPlayer objects before their VlcContext.");
    if (!m_players.isEmpty()) {
        qWarning("VlcContext destroyed while %d media player(s) are still alive; "
                 "destroy players before the context.",
                 static_cast<int>(m_players.size()));
        for (VlcMediaPlayer *player : std::as_const(m_players))
            player->m_context = nullptr;
    }
}

QString VlcContext::version() const
{
    return QString::fromUtf8(libvlc_get_version());
}

void VlcContext::registerPlayer(VlcMediaPlayer *player)
{
    m_players.append(player);
}

void VlcContext::unregisterPlayer(VlcMediaPlayer *player)
{
    m_players.removeAll(player);
}

void VlcContext::InstanceDeleter::operator()(libvlc_instance_t *instance) const
{
    libvlc_release(instance);
}

} // namespace neovlc::backend
