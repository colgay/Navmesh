#include "Utils.h"

#include "extdll.h"
#include "meta_api.h"

void UTIL_ClientPrintAll(int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4)
{
	static int msgTextMsg = 0;

	if (msgTextMsg == 0)
	{
		msgTextMsg = GET_USER_MSG_ID(PLID, "TextMsg", NULL);

		if (!msgTextMsg)
			return;
	}

	MESSAGE_BEGIN(MSG_ALL, msgTextMsg);
	WRITE_BYTE(msg_dest);
	WRITE_STRING(msg_name);
	if (param1)
		WRITE_STRING(param1);
	if (param2)
		WRITE_STRING(param2);
	if (param3)
		WRITE_STRING(param3);
	if (param4)
		WRITE_STRING(param4);
	MESSAGE_END();
}

char* UTIL_VarArgs(const char *format, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, format);
	vsprintf(string, format, argptr);
	va_end(argptr);

	return string;
}

void UTIL_BeamPoints(edict_t *pEntity, const Vector &pos1, const Vector &pos2, short sprite, int startFrame, int frameRate, int life, int width, int noise, int r, int g, int b, int brightness, int speed)
{
	MESSAGE_BEGIN(pEntity == nullptr ? MSG_BROADCAST : MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, NULL, pEntity);
	WRITE_BYTE(TE_BEAMPOINTS);
	WRITE_COORD(pos1.x);
	WRITE_COORD(pos1.y);
	WRITE_COORD(pos1.z);
	WRITE_COORD(pos2.x);
	WRITE_COORD(pos2.y);
	WRITE_COORD(pos2.z);
	WRITE_SHORT(sprite);
	WRITE_BYTE(startFrame);		// startframe
	WRITE_BYTE(frameRate);		// framerate
	WRITE_BYTE(life);		// life
	WRITE_BYTE(width);		// width
	WRITE_BYTE(noise);		// noise
	WRITE_BYTE(r);	// r
	WRITE_BYTE(g);		// g
	WRITE_BYTE(b);		// b
	WRITE_BYTE(brightness);	// brightness
	WRITE_BYTE(speed);		// speed
	MESSAGE_END();
}

float UTIL_DistPointSegment(Vector p, Vector sp0, Vector sp1)
{
	Vector v = sp1 - sp0;
	Vector w = p - sp0;

	float c1 = DotProduct(w, v);
	if (c1 <= 0)
		return (p - sp0).Length();

	float c2 = DotProduct(v, v);
	if (c2 <= c1)
		return (p - sp1).Length();

	float b = c1 / c2;
	Vector pb = sp0 + b * v;

	return (p - pb).Length();
}