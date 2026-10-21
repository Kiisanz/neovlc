#pragma once

#include <QObject>
#include <QString>
#include <memory>

struct libvlc_media_player_t;

namespace neovlc::backend {

class VlcContext;

// Owns one libVLC media player; must be destroyed before its VlcContext.
class VlcMediaPlayer final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ isAvailable CONSTANT)
    Q_PROPERTY(QString errorString READ errorString CONSTANT)

public:
    explicit VlcMediaPlayer(VlcContext &context, QObject *parent = nullptr);
    ~VlcMediaPlayer() override;

    bool isAvailable() const { return m_player != nullptr; }
    QString errorString() const { return m_errorString; }
    libvlc_media_player_t *player() const { return m_player.get(); }

private:
    friend class VlcContext;

    struct PlayerDeleter {
        void operator()(libvlc_media_player_t *player) const;
    };

    VlcContext *m_context;
    std::unique_ptr<libvlc_media_player_t, PlayerDeleter> m_player;
    QString m_errorString;
};

} // namespace neovlc::backend
