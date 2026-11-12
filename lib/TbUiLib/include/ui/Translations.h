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

#pragma once

#include <QHash>
#include <QString>
#include <QTranslator>

#include <string>

class QCoreApplication;

namespace tb::ui
{

/**
 * A translator backed by a flat JSON dictionary of English source strings to
 * translated strings.
 *
 * TrenchBroom's UI strings are wrapped in TB_TR(), which expands to
 * QCoreApplication::translate with a fixed context, so a single dictionary is
 * enough. Entries that are missing from the dictionary fall through to the
 * original English text, which keeps a partially translated language usable.
 */
class JsonTranslator : public QTranslator
{
public:
  JsonTranslator() = default;

  /**
   * Loads translations from the given JSON file, replacing any previously loaded
   * entries. Returns false if the file could not be read or contains no entries.
   */
  bool load(const QString& filePath);

  size_t size() const;

  QString translate(
    const char* context,
    const char* sourceText,
    const char* disambiguation,
    int n) const override;

private:
  QHash<QString, QString> m_entries;
};

/**
 * Returns the language code to use: the explicit argument if non-empty, otherwise
 * the TB_LANG environment variable, otherwise the system UI language, limited to
 * its language part (e.g. "zh" for "zh-CN").
 */
QString selectLanguage(const QString& requested = {});

/**
 * Installs a JsonTranslator for the given language, looking for
 * <application dir>/translations/<language>.json.
 *
 * Returns false when no dictionary exists for that language, in which case the UI
 * stays in English. The translator is owned by the application instance.
 */
bool installTranslation(QCoreApplication& app, const QString& language = {});

/**
 * Translates a UI string coming from the application's own data structures (for
 * example an action label). Strings that are not in the dictionary are returned
 * unchanged, so the UI degrades gracefully to English.
 */
QString translate(const std::string& sourceText);

/**
 * Convenience macro used at call sites that display user visible text. The fixed
 * context keeps the generated dictionary flat.
 */
#define TB_TR(sourceText) \
  QCoreApplication::translate("TrenchBroom", sourceText)

} // namespace tb::ui
