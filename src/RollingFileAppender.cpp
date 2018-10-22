#include <QDateTime>
#include <QDir>
#include <QFileInfo>

#include "RollingFileAppender.h"

namespace CuteLogger
{

	RollingFileAppender::RollingFileAppender(const QString& fileName)
		: FileAppender(fileName),
		m_logFilesLimit(0),
		m_computeRollOverBasedOnLastModified(false),
		m_forceRollovercheck(true)
	{}

	void RollingFileAppender::append(const QDateTime& timeStamp, Logger::LogLevel logLevel, const char* file, int line,
		const char* function, const QString& category, const QString& message)
	{
		if ((QDateTime::currentDateTime() >= m_rollOverTime) || (m_forceRollovercheck)) {
			rollOverCheck();
		}

		FileAppender::append(timeStamp, logLevel, file, line, function, category, message);
	}


	RollingFileAppender::DatePattern RollingFileAppender::datePattern() const
	{
		QMutexLocker locker(&m_rollingMutex);
		return m_frequency;
	}


	QString RollingFileAppender::datePatternString() const
	{
		QMutexLocker locker(&m_rollingMutex);
		return m_datePatternString;
	}


	void RollingFileAppender::setDatePattern(DatePattern datePattern)
	{
		switch (datePattern)
		{
		case MinutelyRollover:
			setDatePatternString(QLatin1String("'.'yyyy-MM-dd-hh-mm"));
			break;
		case HourlyRollover:
			setDatePatternString(QLatin1String("'.'yyyy-MM-dd-hh"));
			break;
		case HalfDailyRollover:
			setDatePatternString(QLatin1String("'.'yyyy-MM-dd-a"));
			break;
		case DailyRollover:
			setDatePatternString(QLatin1String("'.'yyyy-MM-dd"));
			break;
		case WeeklyRollover:
			setDatePatternString(QLatin1String("'.'yyyy-ww"));
			break;
		case MonthlyRollover:
			setDatePatternString(QLatin1String("'.'yyyy-MM"));
			break;
		default:
			Q_ASSERT_X(false, "DailyRollingFileAppender::setDatePattern()", "Invalid datePattern constant");
			setDatePattern(DailyRollover);
		};

		QMutexLocker locker(&m_rollingMutex);
		m_frequency = datePattern;

		computeLogAndRollOverTimes();
	}


	void RollingFileAppender::setDatePattern(const QString& datePattern)
	{
		setDatePatternString(datePattern);
		computeFrequency();

		computeLogAndRollOverTimes();
	}


	void RollingFileAppender::setDatePatternString(const QString& datePatternString)
	{
		QMutexLocker locker(&m_rollingMutex);
		m_datePatternString = datePatternString;
	}


	void RollingFileAppender::computeFrequency()
	{
		QMutexLocker locker(&m_rollingMutex);

		const QDateTime startTime(QDate(1999, 1, 1), QTime(0, 0));
		const QString startString = startTime.toString(m_datePatternString);

		if (startString != startTime.addSecs(60).toString(m_datePatternString))
			m_frequency = MinutelyRollover;
		else if (startString != startTime.addSecs(60 * 60).toString(m_datePatternString))
			m_frequency = HourlyRollover;
		else if (startString != startTime.addSecs(60 * 60 * 12).toString(m_datePatternString))
			m_frequency = HalfDailyRollover;
		else if (startString != startTime.addDays(1).toString(m_datePatternString))
			m_frequency = DailyRollover;
		else if (startString != startTime.addDays(7).toString(m_datePatternString))
			m_frequency = WeeklyRollover;
		else if (startString != startTime.addMonths(1).toString(m_datePatternString))
			m_frequency = MonthlyRollover;
		else
		{
			Q_ASSERT_X(false, "DailyRollingFileAppender::computeFrequency", "The pattern '%1' does not specify a frequency");
			return;
		}
	}


	void RollingFileAppender::removeOldFiles()
	{
		if (m_logFilesLimit <= 1)
			return;

		QFileInfo fileInfo(fileName());
		QDir logDirectory(fileInfo.absoluteDir());
		logDirectory.setFilter(QDir::Files);
		logDirectory.setNameFilters(QStringList() << fileInfo.fileName() + "*");
		QFileInfoList logFiles = logDirectory.entryInfoList();

		QMap<QDateTime, QString> fileDates;
		for (int i = 0; i < logFiles.length(); ++i)
		{
			QString name = logFiles[i].fileName();
			QString suffix = name.mid(name.indexOf(fileInfo.fileName()) + fileInfo.fileName().length());
			QDateTime fileDateTime = QDateTime::fromString(suffix, datePatternString());

			if (fileDateTime.isValid())
				fileDates.insert(fileDateTime, logFiles[i].absoluteFilePath());
		}

		QList<QString> fileDateNames = fileDates.values();
		for (int i = 0; i < fileDateNames.length() - m_logFilesLimit + 1; ++i)
			QFile::remove(fileDateNames[i]);
	}
	
	QDateTime RollingFileAppender::computeFileLogTime() const
	{
		QDateTime dt = QDateTime::currentDateTime();
		if ((m_computeRollOverBasedOnLastModified) && (!fileName().isEmpty())) {
			QFileInfo fi(fileName());
			if (fi.exists()) {
				dt = fi.lastModified();
			}
		}
		return computeRollOverDateTimeHelper(dt, false);
	}

	QDateTime RollingFileAppender::computeCurrentLogTimeNow() const
	{
		return computeRollOverDateTimeHelper(QDateTime::currentDateTime(), false);
	}

	QDateTime RollingFileAppender::computeRollOverDateTimeNext() const
	{
		return computeRollOverDateTimeHelper(QDateTime::currentDateTime(), true);
	}

	QDateTime RollingFileAppender::computeRollOverDateTimeHelper(const QDateTime& dt, bool bNext) const 
	{
		QDate rcDate = dt.date();
		QTime rcTime = dt.time();
		QDateTime rc;
		switch (m_frequency)
		{
		case MinutelyRollover:
		{
			rc = QDateTime(rcDate, QTime(rcTime.hour(), rcTime.minute(), 0, 0));
			if (bNext) {
				rc = rc.addSecs(60);
			}
		}
		break;
		case HourlyRollover:
		{
			rc = QDateTime(rcDate, QTime(rcTime.hour(), 0, 0, 0));
			if (bNext) {
				rc = rc.addSecs(60 * 60);
			}
		}
		break;
		case HalfDailyRollover:
		{
			int hour = rcTime.hour();
			if (hour >= 12)
				hour = 12;
			else
				hour = 0;
			rc = QDateTime(rcDate, QTime(hour, 0, 0, 0));
			if (bNext) {
				rc = rc.addSecs(60 * 60 * 12);
			}
		}
		break;
		case DailyRollover:
		{
			rc = QDateTime(rcDate, QTime(0, 0, 0, 0));
			if (bNext) {
				rc = rc.addDays(1);
			}
		}
		break;
		case WeeklyRollover:
		{
			// Qt numbers the week days 1..7. The week starts on Monday.
			// Change it to being numbered 0..6, starting with Sunday.
			int day = rcDate.dayOfWeek();
			if (day == Qt::Sunday)
				day = 0;
			rc = QDateTime(rcDate, QTime(0, 0, 0, 0)).addDays(-1 * day);
			if (bNext) {
				rc = rc.addDays(7);
			}
		}
		break;
		case MonthlyRollover:
		{
			rc = QDateTime(QDate(rcDate.year(), rcDate.month(), 1), QTime(0, 0, 0, 0));
			if (bNext) {
				rc = rc.addMonths(1);
			}
		}
		break;
		default:
			rc = QDateTime::fromTime_t(0);
			Q_ASSERT_X(false, "DailyRollingFileAppender::computeInterval()", "Invalid datePattern constant");
		}

		return rc;
	}

	void RollingFileAppender::computeLogAndRollOverTimes()
	{
		Q_ASSERT_X(!m_datePatternString.isEmpty(), "DailyRollingFileAppender::computeRollOverTime()", "No active date pattern");

		m_currentLogFileTime = computeFileLogTime();
		m_rollOverTime = computeRollOverDateTimeNext();
	}

	void RollingFileAppender::rollOverCheck()
	{
		Q_ASSERT_X(!m_datePatternString.isEmpty(), "DailyRollingFileAppender::rollOver()", "No active date pattern");

		m_forceRollovercheck = false;

		QString suffixCurrentLogFile = m_currentLogFileTime.toString(m_datePatternString);
		QString suffixNow = computeCurrentLogTimeNow().toString(m_datePatternString);
		if (suffixCurrentLogFile == suffixNow) \
		{
			return;
		}

		closeFile();

		QString targetFileName = fileName() + suffixCurrentLogFile;
		if (QFile::exists(targetFileName) && !QFile::remove(targetFileName))
		{
			return;
		}
		if (!QFile::rename(fileName(), targetFileName))
		{
			return;
		}

		openFile();
		removeOldFiles();
		computeLogAndRollOverTimes();
	}

	void RollingFileAppender::setLogFilesLimit(int limit)
	{
		QMutexLocker locker(&m_rollingMutex);
		m_logFilesLimit = limit;
	}


	int RollingFileAppender::logFilesLimit() const
	{
		QMutexLocker locker(&m_rollingMutex);
		return m_logFilesLimit;
	}

	void RollingFileAppender::setFileName(const QString & f)
	{
		FileAppender::setFileName(f);
		computeLogAndRollOverTimes();
	}


	bool RollingFileAppender::computeRollOverBasedOnLastModified() const
	{
		return m_computeRollOverBasedOnLastModified;
	}

	void RollingFileAppender::setComputeRollOverBasedOnLastModified(bool val)
	{
		m_computeRollOverBasedOnLastModified = val;
	}

}