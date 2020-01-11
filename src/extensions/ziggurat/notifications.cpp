#include "libvolition/include/common.h"
#include "libvolition/include/utils.h"
#include "ziggurat.h"
#include <QtWidgets>
#include <QSound>

#if defined(WIN32) && defined(STATIC)
#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsAudioPlugin)
#endif //WIN32

bool Ziggurat::PlayAudioNotification(const VLString &WavFile)
{
	if ((*WavFile != ':') && !Utils::FileExists(WavFile)) return false;
	
	QSound::play(QString(+WavFile));
	
	return true;
}

