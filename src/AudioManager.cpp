#include "AudioManager.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QtEndian>
#include <cmath>
#include <tuple>

// ---------------------------------------------------------------------------
// WAV format constants
// ---------------------------------------------------------------------------
static constexpr float kPi              = 3.14159265358979323846f;
static constexpr int   kSampleRate      = 44100;
static constexpr int   kChannels        = 1;
static constexpr int   kBitsPerSample   = 16;
static constexpr float kPcmMaxAmplitude = 32767.f;

// Builds the 44-byte standard PCM WAV header for mono 16-bit audio at kSampleRate.
static QByteArray buildWavHeader(int sampleCount)
{
    const int dataSize = sampleCount * kChannels * kBitsPerSample / 8;

    QByteArray header;
    header.reserve(44);

    auto appendBytes = [&](const char *data, int size) { header.append(data, size); };
    auto appendU16   = [&](quint16 v) {
        v = qToLittleEndian(v);
        appendBytes(reinterpret_cast<const char *>(&v), sizeof(v));
    };
    auto appendU32   = [&](quint32 v) {
        v = qToLittleEndian(v);
        appendBytes(reinterpret_cast<const char *>(&v), sizeof(v));
    };

    appendBytes("RIFF", 4);
    appendU32(36 + dataSize);                                   // chunk size
    appendBytes("WAVE", 4);
    appendBytes("fmt ", 4);
    appendU32(16);                                              // PCM fmt chunk size
    appendU16(1);                                               // PCM format tag
    appendU16(kChannels);
    appendU32(kSampleRate);
    appendU32(kSampleRate * kChannels * kBitsPerSample / 8);    // byte rate
    appendU16(kChannels * kBitsPerSample / 8);                  // block align
    appendU16(kBitsPerSample);
    appendBytes("data", 4);
    appendU32(dataSize);

    return header;
}

// ---------------------------------------------------------------------------
// AudioManager
// ---------------------------------------------------------------------------
AudioManager::AudioManager(QObject *parent) : QObject(parent)
{
    const QList<std::tuple<SoundCue, QString, float, int, float>> cues = {
        {SoundCue::Start,          "start",           620.f, 120, 0.24f},
        {SoundCue::Confirm,        "confirm",         480.f,  80, 0.20f},
        {SoundCue::Collect,        "collect",         880.f,  90, 0.20f},
        {SoundCue::CollectSpecial, "collect_special", 1240.f, 130, 0.24f},
        {SoundCue::GameOver,       "game_over",       150.f, 260, 0.28f},
    };

    for (const auto &[cue, name, freq, duration, volume] : cues) {
        const QUrl url = createTone(name, freq, duration, volume);
        QList<QSoundEffect *> voices;
        for (int i = 0; i < 3; ++i) {
            auto *effect = new QSoundEffect(this);
            effect->setSource(url);
            effect->setLoopCount(1);
            effect->setVolume(0.75f);
            voices.append(effect);
        }
        m_effects.insert(cue, voices);
        m_nextVoice.insert(cue, 0);
    }

    m_ambient = new QSoundEffect(this);
    m_ambient->setSource(createAmbient());
    m_ambient->setLoopCount(QSoundEffect::Infinite);
    m_ambient->setVolume(0.16f);
}

void AudioManager::play(SoundCue cue)
{
    QList<QSoundEffect *> voices = m_effects.value(cue);
    if (voices.isEmpty())
        return;

    int index = m_nextVoice.value(cue) % voices.size();
    QSoundEffect *effect = voices[index];
    effect->stop();
    effect->play();
    m_nextVoice[cue] = (index + 1) % voices.size();
}

void AudioManager::startAmbient()
{
    if (m_ambient && !m_ambient->isPlaying())
        m_ambient->play();
}

QUrl AudioManager::createTone(const QString &name, float frequency, int durationMs, float volume)
{
    const QString basePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
        + "/cuarzito-preshow-audio";
    QDir().mkpath(basePath);
    const QString path = basePath + "/" + name + ".wav";

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        file.write(makeWav(frequency, durationMs, volume));

    return QUrl::fromLocalFile(path);
}

QUrl AudioManager::createAmbient()
{
    const QString basePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
        + "/cuarzito-preshow-audio";
    QDir().mkpath(basePath);
    const QString path = basePath + "/ambient.wav";

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        file.write(makeAmbientWav(2000, 0.18f));

    return QUrl::fromLocalFile(path);
}

QByteArray AudioManager::makeWav(float frequency, int durationMs, float volume)
{
    const int sampleCount = kSampleRate * durationMs / 1000;
    const int dataSize    = sampleCount * kChannels * kBitsPerSample / 8;

    QByteArray wav = buildWavHeader(sampleCount);
    wav.reserve(wav.size() + dataSize);

    for (int i = 0; i < sampleCount; ++i) {
        const float t   = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float env = std::sin(kPi * qMin(1.f, static_cast<float>(i) / sampleCount));
        const float s   = std::sin(2.f * kPi * frequency * t) * volume * env;
        qint16 pcm = qToLittleEndian(static_cast<qint16>(s * kPcmMaxAmplitude));
        wav.append(reinterpret_cast<const char *>(&pcm), sizeof(pcm));
    }

    return wav;
}

QByteArray AudioManager::makeAmbientWav(int durationMs, float volume)
{
    const int sampleCount = kSampleRate * durationMs / 1000;
    const int dataSize    = sampleCount * kChannels * kBitsPerSample / 8;

    QByteArray wav = buildWavHeader(sampleCount);
    wav.reserve(wav.size() + dataSize);

    for (int i = 0; i < sampleCount; ++i) {
        const float t       = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float drone   = std::sin(2.f * kPi *  55.f * t) * 0.50f
                            + std::sin(2.f * kPi *  82.5f * t) * 0.28f
                            + std::sin(2.f * kPi * 110.f * t) * 0.18f;
        const float shimmer = std::sin(2.f * kPi * 440.f * t) * 0.035f
                            + std::sin(2.f * kPi * 660.f * t) * 0.020f;
        const float s = qBound(-1.f, (drone + shimmer) * volume, 1.f);
        qint16 pcm = qToLittleEndian(static_cast<qint16>(s * kPcmMaxAmplitude));
        wav.append(reinterpret_cast<const char *>(&pcm), sizeof(pcm));
    }

    return wav;
}
