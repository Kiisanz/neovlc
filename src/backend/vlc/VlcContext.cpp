#include "VlcContext.h"

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

VlcContext::~VlcContext() = default;

QString VlcContext::version() const
{
    return QString::fromUtf8(libvlc_get_version());
}

void VlcContext::InstanceDeleter::operator()(libvlc_instance_t *instance) const
{
    libvlc_release(instance);
}

} // namespace neovlc::backend
