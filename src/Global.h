///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2014 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version, but always including the *additional*
// restrictions defined in the "License.txt" file.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// http://www.gnu.org/licenses/gpl-2.0.txt
///////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef _MSC_VER
#define _CRT_RAND_S
#endif

#include <cstdlib>
#include <QtGlobal>
#include <QString>

//Forward declarations
class QDate;
class QStringList;
class QIcon;
class LockedFile;

///////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

/*
 * Tools Support
 */
void           lamexp_tools_register(const QString &toolName, LockedFile *const file, const quint32 &version, const QString &tag = QString());
bool           lamexp_tools_check   (const QString &toolName);
const QString& lamexp_tools_lookup  (const QString &toolName);
const quint32& lamexp_tools_version (const QString &toolName, QString *const tagOut = NULL);

/*
 * Version getters
 */
unsigned int lamexp_version_major    (void);
unsigned int lamexp_version_minor    (void);
unsigned int lamexp_version_build    (void);
unsigned int lamexp_version_confg    (void);
const char*  lamexp_version_release  (void);
bool         lamexp_version_portable (void);
bool         lamexp_version_demo     (void);
const QDate& lamexp_version_expires  (void);
unsigned int lamexp_toolver_neroaac  (void);
unsigned int lamexp_toolver_fhgaacenc(void);
unsigned int lamexp_toolver_qaacenc  (void);
unsigned int lamexp_toolver_coreaudio(void);

/*
 * URL getters
 */
const char *lamexp_website_url(void);
const char *lamexp_mulders_url(void);
const char *lamexp_support_url(void);
const char *lamexp_tracker_url(void);

/*
 * Misc Functions
 */
const QIcon&  lamexp_app_icon(void);
const QString lamexp_version2string(const QString &pattern, unsigned int version, const QString &defaultText, const QString &tag = QString());

///////////////////////////////////////////////////////////////////////////////
// HELPER MACROS
///////////////////////////////////////////////////////////////////////////////

#define NOBR(STR) (QString("<nobr>%1</nobr>").arg((STR)).replace("-", "&minus;"))
