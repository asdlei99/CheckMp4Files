//----------------------------------------------------------------------------------------------
// MFTime.h
//----------------------------------------------------------------------------------------------
#ifndef MFTIME_H
#define MFTIME_H

// One msec in ns
const LONG ONE_MSEC = 10000;
// One second in ns
const MFTIME ONE_SECOND = 10000000;
// One minute in ns
const MFTIME ONE_MINUTE = 600000000;
// One hour in ns
const MFTIME ONE_HOUR = 36000000000;
// One day in ns
const MFTIME ONE_DAY = 864000000000;

#if (_DEBUG && MF_USE_LOGGING)

inline void MFTraceTimeString(const MFTIME& Duration){

	MFTIME Hours = 0;
	MFTIME Minutes = 0;
	MFTIME Seconds = 0;
	MFTIME MilliSeconds = 0;

	if(Duration < ONE_SECOND){

		MilliSeconds = Duration / ONE_MSEC;
	}
	else if(Duration < ONE_MINUTE){

		Seconds = Duration / ONE_SECOND;
		MilliSeconds = (Duration - (Seconds * ONE_SECOND)) / ONE_MSEC;
	}
	else if(Duration < ONE_HOUR){

		Minutes = Duration / ONE_MINUTE;
		LONGLONG llMinutes = Minutes * ONE_MINUTE;
		Seconds = (Duration - llMinutes) / ONE_SECOND;
		MilliSeconds = (Duration - (llMinutes + (Seconds * ONE_SECOND))) / ONE_MSEC;
	}
	else if(Duration < ONE_DAY){

		Hours = Duration / ONE_HOUR;
		LONGLONG llHours = Hours * ONE_HOUR;
		Minutes = (Duration - llHours) / ONE_MINUTE;
		LONGLONG llMinutes = Minutes * ONE_MINUTE;
		Seconds = (Duration - (llHours + llMinutes)) / ONE_SECOND;
		MilliSeconds = (Duration - (llHours + llMinutes + (Seconds * ONE_SECOND))) / ONE_MSEC;
	}
	else{

		TRACE((L"todo : not implemented (more than one day)"));
		return;
	}

	TRACE((L"%02dh:%02dmn:%02ds:%03dms", (int)Hours, (int)Minutes, (int)Seconds, MilliSeconds));
}

#else
#define MFTraceTimeString(x)
#endif

#endif