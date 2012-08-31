#ifndef TRANSLATORMANAGER_H
#define TRANSLATORMANAGER_H

// translatormanager.h
// 7/1/2012

#include "module/translator/translator.h"
#include <QtCore/QUrl>

class TranslatorManager : public Translator
{
  Q_OBJECT
  Q_DISABLE_COPY(TranslatorManager)
  typedef TranslatorManager Self;
  typedef Translator Base;

  // - Constructions -
public:
  enum ServiceProvider { Romaji = 0, Microsoft, Google, Yahoo, Infoseek,
                         Ocn, Fresheye, Nifty, Excite, Sdl, ServiceCount};
  enum ServiceProviderBit {
    RomajiBit = 1 << Romaji, MicrosoftBit = 1 << Microsoft, GoogleBit = 1 << Google,
    OcnBit = 1 << Ocn, ExciteBit = 1 << Excite, SdlBit = 1 << Sdl, YahooBit = 1 << Yahoo,
    NiftyBit = 1 << Nifty, InfoseekBit = 1 << Infoseek, FresheyeBit = 1 << Fresheye
  };
  enum { AllServices = RomajiBit | MicrosoftBit | GoogleBit | YahooBit | OcnBit | ExciteBit | SdlBit | NiftyBit | InfoseekBit | FresheyeBit};
public:
  explicit TranslatorManager(QObject *parent = nullptr);

  ulong services() const { return services_; }
  bool hasServices() const { return services_; }
  bool hasService(int service) const { return services_ & (1 << service); }

  int serviceCount() const
  {
    int ret = 0;
    for (int i = 0; i < ServiceCount; i++)
      if (hasService(i))
        ret++;
    return ret;
  }

signals:
  void servicesChanged(ulong mask);

  void translatedByRomaji(const QString &text);
  void translatedByMicrosoft(const QString &text);
  void translatedByFresheye(const QString &text);
  void translatedByGoogle(const QString &text);
  void translatedByInfoseek(const QString &text);
  void translatedByOcn(const QString &text);
  void translatedByNifty(const QString &text);
  void translatedByExcite(const QString &text);
  void translatedBySdl(const QString &text);
  void translatedByYahoo(const QString &text);

public slots:
  void setServices(ulong mask)
  { if (services_ != mask) emit servicesChanged(services_ = mask); }

  void setService(int service, bool enable)
  {
    setServices(enable ?
      services_ | (1 << service) :
      services_ & ~(1 << service)
    );
  }

  void translate(const QString &text, const QString &to, const QString &from = QString()) override
  {
    for (int service = 0; service < ServiceCount; service++)
      if (hasService(service))
        translate(service, text, to, from);
  }

  void translate(int service, const QString &text, const QString &to, const QString &from = QString())
  {
    if (!isEnabled())
      return;
    Q_ASSERT(service >= 0 && service < ServiceCount);
    t_[service]->translate(text, to, from);
  }

protected slots:
  void updateSynchronized(bool t);

private:
  Translator *t_[ServiceCount];
  ulong services_;
};

#endif // BINGTRANSLATOR_H
