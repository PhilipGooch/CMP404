#include <platform/vita/system/platform_vita.h>
#include "ar_app.h"

unsigned int sceLibcHeapSize = 128*1024*1024;	// Sets up the heap area size as 128MiB.

#pragma comment(lib, "SceCamera_stub")
#pragma comment(lib, "SceMotion_stub")

int main(void)
{
	// initialisation
	gef::PlatformVita platform;

	ARApp myApp(platform);
	myApp.Run();

	return 0;
}