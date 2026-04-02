#pragma once

#ifdef QUARK_ENGINE
#define QUARK_API __declspec(dllexport)
#else
#define QUARK_API __declspec(dllimport)
#endif // QUARK_ENGINE
