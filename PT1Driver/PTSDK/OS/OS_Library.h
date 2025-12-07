#ifndef _OS_Library_H
#define _OS_Library_H

#include "Prefix.h"
#include "EARTH_PT.h"
#include <windows.h>

namespace EARTH {
namespace OS {
	class Library {
	public:
		Library()
		{
			mModule = NULL;
			mFunction = NULL;

			wchar_t path[MAX_PATH];

			#if defined(ENV_EARTHSOFT)
				const wchar_t *configuration;
				#if defined(_DEBUG)
					configuration = L"Debug";
				#else
					configuration = L"Release";
				#endif

				const wchar_t *bit;
				#if !defined(_WIN64)
					bit = L"32bit";
				#else
					bit = L"64bit";
				#endif

				::wsprintf(path, L"E:\\Software\\PT1\\Package\\%s\\%s\\SDK\\SDK.dll", configuration, bit);
			#else
				::wsprintf(path, L"SDK_EARTHSOFT_PT1.dll");		
			#endif

			_ASSERTE(mModule == NULL);
			mModule = ::LoadLibrary(path);
			if (mModule == NULL) { _ASSERTE(false); return; }

			mFunction = reinterpret_cast<PT::Bus::NewBusFunction>(::GetProcAddress(mModule, "_"));
			if (mFunction == NULL) { _ASSERTE(false); return; }
		}

		~Library()
		{
			if (mModule) {
				BOOL b = ::FreeLibrary(mModule);
				if (b) {
					mModule = NULL;
				}
			}

			mFunction = NULL;
		}

		PT::Bus::NewBusFunction Function() const
		{
			return mFunction;
		}

	protected:
		HMODULE mModule;
		PT::Bus::NewBusFunction mFunction;
	};
}
}

#endif
