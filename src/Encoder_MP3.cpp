///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2016 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Encoder_MP3.h"

#include "Global.h"
#include "Model_Settings.h"

#include <QProcess>
#include <QDir>
#include <limits.h>

static const int g_lameAgorithmQualityLUT[5] = {7, 5, 2, 0, INT_MAX};
static const int g_mp3BitrateLUT[15] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1};
static const int g_lameVBRQualityLUT[11] = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0, INT_MAX};

///////////////////////////////////////////////////////////////////////////////
// Encoder Info
///////////////////////////////////////////////////////////////////////////////

class MP3EncoderInfo : public AbstractEncoderInfo
{
	virtual bool isModeSupported(int mode) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return true;
			break;
		default:
			MUTILS_THROW("Bad RC mode specified!");
		}
	}

	virtual int valueCount(int mode) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
			return 10;
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return 14;
			break;
		default:
			MUTILS_THROW("Bad RC mode specified!");
		}
	}

	virtual int valueAt(int mode, int index) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
			return g_lameVBRQualityLUT[qBound(0, index, 9)];
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return g_mp3BitrateLUT[qBound(0, index, 13)];
			break;
		default:
			MUTILS_THROW("Bad RC mode specified!");
		}
	}

	virtual int valueType(int mode) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
			return TYPE_QUALITY_LEVEL_INT;
			break;
		case SettingsModel::ABRMode:
			return TYPE_APPROX_BITRATE;
			break;
		case SettingsModel::CBRMode:
			return TYPE_BITRATE;
			break;
		default:
			MUTILS_THROW("Bad RC mode specified!");
		}
	}

	virtual const char *description(void) const
	{
		static const char* s_description = "LAME MP3 Encoder";
		return s_description;
	}

	virtual const char *extension(void) const
	{
		static const char* s_extension = "mp3";
		return s_extension;
	}

	virtual bool isResamplingSupported(void) const
	{
		return true;
	}
}
static const g_mp3EncoderInfo;

///////////////////////////////////////////////////////////////////////////////
// Encoder implementation
///////////////////////////////////////////////////////////////////////////////

MP3Encoder::MP3Encoder(void)
:
	m_binary(lamexp_tools_lookup(L1S("lame.exe")))
{
	if(m_binary.isEmpty())
	{
		MUTILS_THROW("Error initializing MP3 encoder. Tool 'lame.exe' is not registred!");
	}
	
	m_algorithmQuality = 2;
	m_configBitrateMaximum = 0;
	m_configBitrateMinimum = 0;
	m_configSamplingRate = 0;
	m_configChannelMode = 0;
}

MP3Encoder::~MP3Encoder(void)
{
}

bool MP3Encoder::encode(const QString &sourceFile, const AudioFileModel_MetaInfo &metaInfo, const unsigned int duration, const unsigned int channels, const QString &outputFile, volatile bool *abortFlag)
{
	QProcess process;
	QStringList args;

	args << L1S("--nohist");
	args << L1S("-q") << QString::number(g_lameAgorithmQualityLUT[m_algorithmQuality]);
		
	switch(m_configRCMode)
	{
	case SettingsModel::VBRMode:
		args << L1S("-V") << QString::number(g_lameVBRQualityLUT[qBound(0, m_configBitrate, 9)]);
		break;
	case SettingsModel::ABRMode:
		args << L1S("--abr") << QString::number(g_mp3BitrateLUT[qBound(0, m_configBitrate, 13)]);
		break;
	case SettingsModel::CBRMode:
		args << L1S("--cbr");
		args << L1S("-b") << QString::number(g_mp3BitrateLUT[qBound(0, m_configBitrate, 13)]);
		break;
	default:
		MUTILS_THROW("Bad rate-control mode!");
		break;
	}

	if((m_configBitrateMaximum > 0) && (m_configBitrateMinimum > 0) && (m_configBitrateMinimum <= m_configBitrateMaximum))
	{
		if(m_configRCMode != SettingsModel::CBRMode)
		{
			args << L1S("-b") << QString::number(clipBitrate(m_configBitrateMinimum));
			args << L1S("-B") << QString::number(clipBitrate(m_configBitrateMaximum));
		}
	}

	if(m_configSamplingRate > 0)
	{
		args << L1S("--resample") << QString::number(m_configSamplingRate);
	}

	switch(m_configChannelMode)
	{
	case 1:
		args << L1S("-m") << L1S("j");
		break;
	case 2:
		args << L1S("-m") << L1S("f");
		break;
	case 3:
		args << L1S("-m") << L1S("s");
		break;
	case 4:
		args << L1S("-m") << L1S("d");
		break;
	case 5:
		args << L1S("-m") << L1S("m");
		break;
	}

	bool bUseUCS2 = false;

	if(isUnicode(metaInfo.title()))   bUseUCS2 = true;
	if(isUnicode(metaInfo.artist()))  bUseUCS2 = true;
	if(isUnicode(metaInfo.album()))   bUseUCS2 = true;
	if(isUnicode(metaInfo.genre()))   bUseUCS2 = true;
	if(isUnicode(metaInfo.comment())) bUseUCS2 = true;

	if (bUseUCS2) args << L1S("--id3v2-ucs2"); //Must specify this BEFORE "--tt" and friends!

	if(!metaInfo.title().isEmpty())   args << L1S("--tt") << cleanTag(metaInfo.title());
	if(!metaInfo.artist().isEmpty())  args << L1S("--ta") << cleanTag(metaInfo.artist());
	if(!metaInfo.album().isEmpty())   args << L1S("--tl") << cleanTag( metaInfo.album());
	if(!metaInfo.genre().isEmpty())   args << L1S("--tg") << cleanTag(metaInfo.genre());
	if(!metaInfo.comment().isEmpty()) args << L1S("--tc") << cleanTag(metaInfo.comment());
	if(metaInfo.year())               args << L1S("--ty") << QString::number(metaInfo.year());
	if(metaInfo.position())           args << L1S("--tn") << QString::number(metaInfo.position());
	if(!metaInfo.cover().isEmpty())   args << L1S("--ti") << QDir::toNativeSeparators(metaInfo.cover());

	if(!m_configCustomParams.isEmpty()) args << m_configCustomParams.split(" ", QString::SkipEmptyParts);

	args << QDir::toNativeSeparators(sourceFile);
	args << QDir::toNativeSeparators(outputFile);

	if(!startProcess(process, m_binary, args))
	{
		return false;
	}

	bool bTimeout = false;
	bool bAborted = false;
	int prevProgress = -1;

	QRegExp regExp(L1S("\\(.*(\\d+)%\\)\\|"));

	while(process.state() != QProcess::NotRunning)
	{
		if(*abortFlag)
		{
			process.kill();
			bAborted = true;
			emit messageLogged(L1S("\nABORTED BY USER !!!"));
			break;
		}
		process.waitForReadyRead(m_processTimeoutInterval);
		if(!process.bytesAvailable() && process.state() == QProcess::Running)
		{
			process.kill();
			qWarning("LAME process timed out <-- killing!");
			emit messageLogged(L1S("\nPROCESS TIMEOUT !!!"));
			bTimeout = true;
			break;
		}
		while(process.bytesAvailable() > 0)
		{
			QByteArray line = process.readLine();
			QString text = QString::fromUtf8(line.constData()).simplified();
			if(regExp.lastIndexIn(text) >= 0)
			{
				bool ok = false;
				int progress = regExp.cap(1).toInt(&ok);
				if(ok && (progress > prevProgress))
				{
					emit statusUpdated(progress);
					prevProgress = qMin(progress + 2, 99);
				}
			}
			else if(!text.isEmpty())
			{
				emit messageLogged(text);
			}
		}
	}

	process.waitForFinished();
	if(process.state() != QProcess::NotRunning)
	{
		process.kill();
		process.waitForFinished(-1);
	}
	
	emit statusUpdated(100);
	emit messageLogged(QString().sprintf("\nExited with code: 0x%04X", process.exitCode()));

	if(bTimeout || bAborted || process.exitCode() != EXIT_SUCCESS)
	{
		return false;
	}
	
	return true;
}

bool MP3Encoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
{
	if(containerType.compare(L1S("Wave"), Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare(L1S("PCM"), Qt::CaseInsensitive) == 0)
		{
			return true;
		}
	}
	else if(containerType.compare(L1S("MPEG Audio"), Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare(L1S("MPEG Audio"), Qt::CaseInsensitive) == 0)
		{
			if(formatProfile.compare(L1S("Layer 3"), Qt::CaseInsensitive) == 0 || formatProfile.compare(L1S("Layer 2"), Qt::CaseInsensitive) == 0)
			{
				if(formatVersion.compare(L1S("Version 1"), Qt::CaseInsensitive) == 0 || formatVersion.compare(L1S("Version 2"), Qt::CaseInsensitive) == 0)
				{
					return true;
				}
			}
		}
	}

	return false;
}

const unsigned int *MP3Encoder::supportedChannelCount(void)
{
	static const unsigned int supportedChannels[] = {1, 2, NULL};
	return supportedChannels;
}

void MP3Encoder::setAlgoQuality(int value)
{
	m_algorithmQuality = qBound(0, value, 3);
}

void MP3Encoder::setBitrateLimits(int minimumBitrate, int maximumBitrate)
{
	m_configBitrateMinimum = minimumBitrate;
	m_configBitrateMaximum = maximumBitrate;
}

void MP3Encoder::setChannelMode(int value)
{
	m_configChannelMode = value;
}

int MP3Encoder::clipBitrate(int bitrate)
{
	int targetBitrate = qMin(qMax(bitrate, 32), 320);
	
	int minDiff = INT_MAX;
	int minIndx = -1;

	for(int i = 0; g_mp3BitrateLUT[i] > 0; i++)
	{
		int currentDiff = abs(targetBitrate - g_mp3BitrateLUT[i]);
		if(currentDiff < minDiff)
		{
			minDiff = currentDiff;
			minIndx = i;
		}
	}

	if(minIndx >= 0)
	{
		return g_mp3BitrateLUT[minIndx];
	}

	return targetBitrate;
}

const AbstractEncoderInfo *MP3Encoder::getEncoderInfo(void)
{
	return &g_mp3EncoderInfo;
}
