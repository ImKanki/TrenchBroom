/*
 Copyright (C) 2010 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ui/Translations.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLocale>
#include <QProcessEnvironment>
#include <QStringList>

namespace tb::ui
{

bool JsonTranslator::load(const QString& filePath)
{
  auto file = QFile{filePath};
  if (!file.open(QIODevice::ReadOnly))
  {
    return false;
  }

  const auto document = QJsonDocument::fromJson(file.readAll());
  if (!document.isObject())
  {
    return false;
  }

  m_entries.clear();
  const auto object = document.object();
  for (auto it = object.begin(); it != object.end(); ++it)
  {
    if (it.value().isString())
    {
      m_entries.insert(it.key(), it.value().toString());
    }
  }
  return !m_entries.isEmpty();
}

size_t JsonTranslator::size() const
{
  return static_cast<size_t>(m_entries.size());
}

QString JsonTranslator::translate(
  const char* context, const char* sourceText, const char* disambiguation, int n) const
{
  // Only the source text matters; context, disambiguation and n are ignored so the
  // dictionary can stay flat.
  if (sourceText == nullptr)
  {
    return {};
  }

  const auto key = QString::fromUtf8(sourceText);
  const auto it = m_entries.constFind(key);
  if (it != m_entries.constEnd())
  {
    return it.value();
  }
  // An empty result tells Qt to fall back to the source string.
  return {};
}

namespace
{

QString normalizedLanguage(const QString& language)
{
  auto result = language.trimmed();
  result.replace('-', '_');
  const auto separator = result.indexOf('_');
  if (separator > 0)
  {
    result = result.left(separator);
  }
  return result;
}

QString translationFileFor(const QString& language)
{
  const auto dir = QDir{QCoreApplication::applicationDirPath() + "/translations"};
  return dir.filePath(language + ".json");
}

void debugLog(const QString& message)
{
  auto file = QFile{QCoreApplication::applicationDirPath() + "/translation-debug.log"};
  if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
  {
    file.write((message + "\n").toUtf8());
  }
}

} // namespace

QString translate(const std::string& sourceText)
{
  return QCoreApplication::translate("TrenchBroom", sourceText.c_str());
}

QString translate(const char* sourceText)
{
  return QCoreApplication::translate("TrenchBroom", sourceText);
}

QString selectLanguage(const QString& requested)
{
  if (!requested.isEmpty())
  {
    return normalizedLanguage(requested);
  }

  const auto environment = QProcessEnvironment::systemEnvironment();
  if (environment.contains("TB_LANG"))
  {
    return normalizedLanguage(environment.value("TB_LANG"));
  }

  // Follow the UI language of the system, but only the language part.
  const auto locale = QLocale::system();
  const auto candidates = locale.uiLanguages();
  if (!candidates.isEmpty())
  {
    return normalizedLanguage(candidates.first());
  }
  return normalizedLanguage(locale.name());
}

bool installTranslation(QCoreApplication& app, const QString& language)
{
  const auto code = selectLanguage(language);
  debugLog(QString{"selectLanguage -> '%1' (requested: '%2')"}.arg(code, language));
  if (code.isEmpty() || code == "en")
  {
    debugLog("no language selected, staying with English");
    return false;
  }

  // Try the full locale first (zh_CN.json), then the bare language (zh.json), so both
  // naming schemes work.
  auto candidates = QStringList{};
  const auto requestedLanguage = language.trimmed().replace('-', '_');
  if (!requestedLanguage.isEmpty())
  {
    candidates << requestedLanguage;
  }
  const auto localeName = QLocale::system().name();
  if (!localeName.isEmpty() && !candidates.contains(localeName))
  {
    candidates << localeName;
  }
  if (!candidates.contains(code))
  {
    candidates << code;
  }

  for (const auto& candidate : candidates)
  {
    const auto path = translationFileFor(candidate);
    const auto exists = QFile::exists(path);
    debugLog(QString{"trying '%1' exists=%2"}.arg(path).arg(exists ? "yes" : "no"));
    if (!exists)
    {
      continue;
    }

    auto* translator = new JsonTranslator{};
    if (!translator->load(path))
    {
      debugLog("load failed");
      delete translator;
      continue;
    }
    debugLog(QString{"loaded %1 entries"}.arg(static_cast<qulonglong>(translator->size())));
    const auto installed = app.installTranslator(translator);
    debugLog(QString{"installTranslator -> %1"}.arg(installed ? "true" : "false"));
    if (installed)
    {
      return true;
    }
    delete translator;
  }

  debugLog("no usable dictionary found");
  return false;
}

} // namespace tb::ui
