#pragma once

#include <QObject>
#include <QString>
#include <memory>

struct libvlc_instance_t;

namespace neovlc::backend {

// Owns the shared libVLC instance; future media players must be destroyed first.
class VlcContext final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ isAvailable CONSTANT)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString errorString READ errorString CONSTANT)

public:
    explicit VlcContext(QObject *parent = nullptr);
    ~VlcContext() override;

    bool isAvailable() const { return m_instance != nullptr; }
    QString version() const;
    QString errorString() const { return m_errorString; }
    libvlc_instance_t *instance() const { return m_instance.get(); }

private:
    struct InstanceDeleter {
        void operator()(libvlc_instance_t *instance) const;
    };

    std::unique_ptr<libvlc_instance_t, InstanceDeleter> m_instance;
    QString m_errorString;
};

} // namespace neovlc::backend
