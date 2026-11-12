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

} // namespace

QString translate(const std::string& sourceText)
{
  return QCoreApplication::translate("TrenchBroom", sourceText.c_str());
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
  if (code.isEmpty() || code == "en")
  {
    return false;
  }

  auto* translator = new JsonTranslator{};
  if (!translator->load(translationFileFor(code)))
  {
    delete translator;
    return false;
  }

  // The translator lives for the rest of the process and is intentionally leaked
  // into the application; removing it would require tracking it explicitly.
  return app.installTranslator(translator);
}

} // namespace tb::ui
