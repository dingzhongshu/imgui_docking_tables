// dear imgui, v1.79 WIP
// (drawing and font code)

/*

Index of this file:

// [SECTION] STB libraries implementation
// [SECTION] Style functions
// [SECTION] ImDrawList
// [SECTION] ImDrawListSplitter
// [SECTION] ImDrawData
// [SECTION] Helpers ShadeVertsXXX functions
// [SECTION] ImFontConfig
// [SECTION] ImFontAtlas
// [SECTION] ImFontAtlas glyph ranges helpers
// [SECTION] ImFontGlyphRangesBuilder
// [SECTION] ImFont
// [SECTION] ImGui Internal Render Helpers
// [SECTION] Decompression code
// [SECTION] Default font data (ProggyClean.ttf)

*/

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui.h"
#ifndef IMGUI_DISABLE

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"

#include <stdio.h>      // vsnprintf, sscanf, printf
#if !defined(alloca)
#if defined(__GLIBC__) || defined(__sun) || defined(__APPLE__) || defined(__NEWLIB__)
#include <alloca.h>     // alloca (glibc uses <alloca.h>. Note that Cygwin may have _WIN32 defined, so the order matters here)
#elif defined(_WIN32)
#include <malloc.h>     // alloca
#if !defined(alloca)
#define alloca _alloca  // for clang with MS Codegen
#endif
#else
#include <stdlib.h>     // alloca
#endif
#endif

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4127) // condition expression is constant
#pragma warning (disable: 4505) // unreferenced local function has been removed (stb stuff)
#pragma warning (disable: 4996) // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#endif

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#if __has_warning("-Wunknown-warning-option")
#pragma clang diagnostic ignored "-Wunknown-warning-option"         // warning: unknown warning group 'xxx'                      // not all warnings are known by all Clang versions and they tend to be rename-happy.. so ignoring warnings triggers new warnings on some configuration. Great!
#endif
#pragma clang diagnostic ignored "-Wunknown-pragmas"                // warning: unknown warning group 'xxx'
#pragma clang diagnostic ignored "-Wold-style-cast"                 // warning: use of old-style cast                            // yes, they are more terse.
#pragma clang diagnostic ignored "-Wfloat-equal"                    // warning: comparing floating point with == or != is unsafe // storing and comparing against same constants ok.
#pragma clang diagnostic ignored "-Wglobal-constructors"            // warning: declaration requires a global destructor         // similar to above, not sure what the exact difference is.
#pragma clang diagnostic ignored "-Wsign-conversion"                // warning: implicit conversion changes signedness
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"  // warning: zero as null pointer constant                    // some standard header variations use #define NULL 0
#pragma clang diagnostic ignored "-Wcomma"                          // warning: possible misuse of comma operator here
#pragma clang diagnostic ignored "-Wreserved-id-macro"              // warning: macro name is a reserved identifier
#pragma clang diagnostic ignored "-Wdouble-promotion"               // warning: implicit conversion from 'float' to 'double' when passing argument to function  // using printf() is a misery with this as C++ va_arg ellipsis changes float to double.
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"  // warning: implicit conversion from 'xxx' to 'float' may lose precision
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wpragmas"                  // warning: unknown option after '#pragma GCC diagnostic' kind
#pragma GCC diagnostic ignored "-Wunused-function"          // warning: 'xxxx' defined but not used
#pragma GCC diagnostic ignored "-Wdouble-promotion"         // warning: implicit conversion from 'float' to 'double' when passing argument to function
#pragma GCC diagnostic ignored "-Wconversion"               // warning: conversion to 'xxxx' from 'xxxx' may alter its value
#pragma GCC diagnostic ignored "-Wstack-protector"          // warning: stack protector not protecting local variables: variable length buffer
#pragma GCC diagnostic ignored "-Wclass-memaccess"          // [__GNUC__ >= 8] warning: 'memset/memcpy' clearing/writing an object of type 'xxxx' with no trivial copy-assignment; use assignment or value-initialization instead
#endif

//-------------------------------------------------------------------------
// [SECTION] STB libraries implementation
//-------------------------------------------------------------------------

// Compile time options:
//#define IMGUI_STB_NAMESPACE           ImStb
//#define IMGUI_STB_TRUETYPE_FILENAME   "my_folder/stb_truetype.h"
//#define IMGUI_STB_RECT_PACK_FILENAME  "my_folder/stb_rect_pack.h"
//#define IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION
//#define IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION

#ifdef IMGUI_STB_NAMESPACE
namespace IMGUI_STB_NAMESPACE
{
#endif

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4456)                             // declaration of 'xx' hides previous local declaration
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wmissing-prototypes"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wcast-qual"              // warning: cast from 'const xxxx *' to 'xxx *' drops const qualifier
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"              // warning: comparison is always true due to limited range of data type [-Wtype-limits]
#pragma GCC diagnostic ignored "-Wcast-qual"                // warning: cast from type 'const xxxx *' to type 'xxxx *' casts away qualifiers
#endif

#ifndef STB_RECT_PACK_IMPLEMENTATION                        // in case the user already have an implementation in the _same_ compilation unit (e.g. unity builds)
#ifndef IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION
#define STBRP_STATIC
#define STBRP_ASSERT(x)     do { IM_ASSERT(x); } while (0)
#define STBRP_SORT          ImQsort
#define STB_RECT_PACK_IMPLEMENTATION
#endif
#ifdef IMGUI_STB_RECT_PACK_FILENAME
#include IMGUI_STB_RECT_PACK_FILENAME
#else
#include "imstb_rectpack.h"
#endif
#endif

#ifndef STB_TRUETYPE_IMPLEMENTATION                         // in case the user already have an implementation in the _same_ compilation unit (e.g. unity builds)
#ifndef IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION
#define STBTT_malloc(x,u)   ((void)(u), IM_ALLOC(x))
#define STBTT_free(x,u)     ((void)(u), IM_FREE(x))
#define STBTT_assert(x)     do { IM_ASSERT(x); } while(0)
#define STBTT_fmod(x,y)     ImFmod(x,y)
#define STBTT_sqrt(x)       ImSqrt(x)
#define STBTT_pow(x,y)      ImPow(x,y)
#define STBTT_fabs(x)       ImFabs(x)
#define STBTT_ifloor(x)     ((int)ImFloorStd(x))
#define STBTT_iceil(x)      ((int)ImCeil(x))
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#else
#define STBTT_DEF extern
#endif
#ifdef IMGUI_STB_TRUETYPE_FILENAME
#include IMGUI_STB_TRUETYPE_FILENAME
#else
#include "imstb_truetype.h"
#endif
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#if defined(_MSC_VER)
#pragma warning (pop)
#endif

#include <algorithm>

#ifdef IMGUI_STB_NAMESPACE
} // namespace ImStb
using namespace IMGUI_STB_NAMESPACE;
#endif

//-----------------------------------------------------------------------------
// [SECTION] Style functions
//-----------------------------------------------------------------------------

void ImGui::StyleColorsDark(ImGuiStyle* dst)
{
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header],       colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_TabActive]              = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabUnfocused]           = ImLerp(colors[ImGuiCol_Tab],          colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImLerp(colors[ImGuiCol_TabActive],    colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_DockingPreview]         = colors[ImGuiCol_HeaderActive] * ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

void ImGui::StyleColorsClassic(ImGuiStyle* dst)
{
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.70f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.11f, 0.11f, 0.14f, 0.92f);
    colors[ImGuiCol_Border]                 = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.43f, 0.43f, 0.43f, 0.39f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.47f, 0.47f, 0.69f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.42f, 0.41f, 0.64f, 0.69f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.27f, 0.27f, 0.54f, 0.83f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.32f, 0.32f, 0.63f, 0.87f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.40f, 0.40f, 0.55f, 0.80f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.20f, 0.25f, 0.30f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.40f, 0.40f, 0.80f, 0.30f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.80f, 0.40f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.41f, 0.39f, 0.80f, 0.60f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.41f, 0.39f, 0.80f, 0.60f);
    colors[ImGuiCol_Button]                 = ImVec4(0.35f, 0.40f, 0.61f, 0.62f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.40f, 0.48f, 0.71f, 0.79f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.46f, 0.54f, 0.80f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.40f, 0.40f, 0.90f, 0.45f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.45f, 0.45f, 0.90f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.53f, 0.53f, 0.87f, 0.80f);
    colors[ImGuiCol_Separator]              = ImVec4(0.50f, 0.50f, 0.50f, 0.60f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.60f, 0.60f, 0.70f, 1.00f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.70f, 0.70f, 0.90f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(1.00f, 1.00f, 1.00f, 0.16f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.78f, 0.82f, 1.00f, 0.60f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.78f, 0.82f, 1.00f, 0.90f);
    colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header],       colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_TabActive]              = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabUnfocused]           = ImLerp(colors[ImGuiCol_Tab],          colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImLerp(colors[ImGuiCol_TabActive],    colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_DockingPreview]         = colors[ImGuiCol_Header] * ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.27f, 0.27f, 0.38f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.45f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.26f, 0.26f, 0.28f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

// Those light colors are better suited with a thicker font than the default one + FrameBorder
void ImGui::StyleColorsLight(ImGuiStyle* dst)
{
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text]                   = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
    colors[ImGuiCol_Border]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.46f, 0.54f, 0.80f, 0.60f);
    colors[ImGuiCol_Button]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.39f, 0.39f, 0.39f, 0.62f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.80f, 0.80f, 0.80f, 0.56f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab]                    = ImLerp(colors[ImGuiCol_Header],       colors[ImGuiCol_TitleBgActive], 0.90f);
    colors[ImGuiCol_TabHovered]             = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_TabActive]              = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabUnfocused]           = ImLerp(colors[ImGuiCol_Tab],          colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImLerp(colors[ImGuiCol_TabActive],    colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_DockingPreview]         = colors[ImGuiCol_Header] * ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.45f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.78f, 0.87f, 0.98f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.57f, 0.57f, 0.64f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.68f, 0.68f, 0.74f, 1.00f);   // Prefer using Alpha=1.0 here
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(0.30f, 0.30f, 0.30f, 0.07f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_NavHighlight]           = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

//-----------------------------------------------------------------------------
// ImDrawList
//-----------------------------------------------------------------------------

ImDrawListSharedData::ImDrawListSharedData()
{
    Font = NULL;
    FontSize = 0.0f;
    CurveTessellationTol = 0.0f;
    CircleSegmentMaxError = 0.0f;
    ClipRectFullscreen = ImVec4(-8192.0f, -8192.0f, +8192.0f, +8192.0f);
    InitialFlags = ImDrawListFlags_None;

    // Lookup tables
    for (int i = 0; i < IM_ARRAYSIZE(ArcFastVtx); i++)
    {
        const float a = ((float)i * 2 * IM_PI) / (float)IM_ARRAYSIZE(ArcFastVtx);
        ArcFastVtx[i] = ImVec2(ImCos(a), ImSin(a));
    }
    memset(CircleSegmentCounts, 0, sizeof(CircleSegmentCounts)); // This will be set by SetCircleSegmentMaxError()
    TexUvLines = NULL;
}

void ImDrawListSharedData::SetCircleSegmentMaxError(float max_error)
{
    if (CircleSegmentMaxError == max_error)
        return;
    CircleSegmentMaxError = max_error;
    for (int i = 0; i < IM_ARRAYSIZE(CircleSegmentCounts); i++)
    {
        const float radius = i + 1.0f;
        const int segment_count = IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC(radius, CircleSegmentMaxError);
        CircleSegmentCounts[i] = (ImU8)ImMin(segment_count, 255);
    }
}

// Initialize before use in a new frame. We always have a command ready in the buffer.
void ImDrawList::_ResetForNewFrame()
{
    // Verify that the ImDrawCmd fields we want to memcmp() are contiguous in memory.
    // (those should be IM_STATIC_ASSERT() in theory but with our pre C++11 setup the whole check doesn't compile with GCC)
    IM_ASSERT(IM_OFFSETOF(ImDrawCmd, ClipRect) == 0);
    IM_ASSERT(IM_OFFSETOF(ImDrawCmd, TextureId) == sizeof(ImVec4));
    IM_ASSERT(IM_OFFSETOF(ImDrawCmd, VtxOffset) == sizeof(ImVec4) + sizeof(ImTextureID));

    CmdBuffer.resize(0);
    IdxBuffer.resize(0);
    VtxBuffer.resize(0);
    Flags = _Data->InitialFlags;
    memset(&_CmdHeader, 0, sizeof(_CmdHeader));
    _VtxCurrentIdx = 0;
    _VtxWritePtr = NULL;
    _IdxWritePtr = NULL;
    _ClipRectStack.resize(0);
    _TextureIdStack.resize(0);
    _Path.resize(0);
    _Splitter.Clear();
    CmdBuffer.push_back(ImDrawCmd());
}

void ImDrawList::_ClearFreeMemory()
{
    CmdBuffer.clear();
    IdxBuffer.clear();
    VtxBuffer.clear();
    Flags = ImDrawListFlags_None;
    _VtxCurrentIdx = 0;
    _VtxWritePtr = NULL;
    _IdxWritePtr = NULL;
    _ClipRectStack.clear();
    _TextureIdStack.clear();
    _Path.clear();
    _Splitter.ClearFreeMemory();
}

ImDrawList* ImDrawList::CloneOutput() const
{
    ImDrawList* dst = IM_NEW(ImDrawList(_Data));
    dst->CmdBuffer = CmdBuffer;
    dst->IdxBuffer = IdxBuffer;
    dst->VtxBuffer = VtxBuffer;
    dst->Flags = Flags;
    return dst;
}

void ImDrawList::AddDrawCmd()
{
    ImDrawCmd draw_cmd;
    draw_cmd.ClipRect = _CmdHeader.ClipRect;    // Same as calling ImDrawCmd_HeaderCopy()
    draw_cmd.TextureId = _CmdHeader.TextureId;
    draw_cmd.VtxOffset = _CmdHeader.VtxOffset;
    draw_cmd.IdxOffset = IdxBuffer.Size;

    IM_ASSERT(draw_cmd.ClipRect.x <= draw_cmd.ClipRect.z && draw_cmd.ClipRect.y <= draw_cmd.ClipRect.w);
    CmdBuffer.push_back(draw_cmd);
}

// Pop trailing draw command (used before merging or presenting to user)
// Note that this leaves the ImDrawList in a state unfit for further commands, as most code assume that CmdBuffer.Size > 0 && CmdBuffer.back().UserCallback == NULL
void ImDrawList::_PopUnusedDrawCmd()
{
    if (CmdBuffer.Size == 0)
        return;
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    if (curr_cmd->ElemCount == 0 && curr_cmd->UserCallback == NULL)
        CmdBuffer.pop_back();
}

void ImDrawList::AddCallback(ImDrawCallback callback, void* callback_data)
{
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    IM_ASSERT(curr_cmd->UserCallback == NULL);
    if (curr_cmd->ElemCount != 0)
    {
        AddDrawCmd();
        curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    }
    curr_cmd->UserCallback = callback;
    curr_cmd->UserCallbackData = callback_data;

    AddDrawCmd(); // Force a new command after us (see comment below)
}

// Compare ClipRect, TextureId and VtxOffset with a single memcmp()
#define ImDrawCmd_HeaderSize                        (IM_OFFSETOF(ImDrawCmd, VtxOffset) + sizeof(unsigned int))
#define ImDrawCmd_HeaderCompare(CMD_LHS, CMD_RHS)   (memcmp(CMD_LHS, CMD_RHS, ImDrawCmd_HeaderSize))    // Compare ClipRect, TextureId, VtxOffset
#define ImDrawCmd_HeaderCopy(CMD_DST, CMD_SRC)      (memcpy(CMD_DST, CMD_SRC, ImDrawCmd_HeaderSize))    // Copy ClipRect, TextureId, VtxOffset

// Our scheme may appears a bit unusual, basically we want the most-common calls AddLine AddRect etc. to not have to perform any check so we always have a command ready in the stack.
// The cost of figuring out if a new command has to be added or if we can merge is paid in those Update** functions only.
void ImDrawList::_OnChangedClipRect()
{
    // If current command is used with different settings we need to add a new command
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    if (curr_cmd->ElemCount != 0 && memcmp(&curr_cmd->ClipRect, &_CmdHeader.ClipRect, sizeof(ImVec4)) != 0)
    {
        AddDrawCmd();
        return;
    }
    IM_ASSERT(curr_cmd->UserCallback == NULL);

    // Try to merge with previous command if it matches, else use current command
    ImDrawCmd* prev_cmd = curr_cmd - 1;
    if (curr_cmd->ElemCount == 0 && CmdBuffer.Size > 1 && ImDrawCmd_HeaderCompare(&_CmdHeader, prev_cmd) == 0 && prev_cmd->UserCallback == NULL)
    {
        CmdBuffer.pop_back();
        return;
    }

    curr_cmd->ClipRect = _CmdHeader.ClipRect;
}

void ImDrawList::_OnChangedTextureID()
{
    // If current command is used with different settings we need to add a new command
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    if (curr_cmd->ElemCount != 0 && curr_cmd->TextureId != _CmdHeader.TextureId)
    {
        AddDrawCmd();
        return;
    }
    IM_ASSERT(curr_cmd->UserCallback == NULL);

    // Try to merge with previous command if it matches, else use current command
    ImDrawCmd* prev_cmd = curr_cmd - 1;
    if (curr_cmd->ElemCount == 0 && CmdBuffer.Size > 1 && ImDrawCmd_HeaderCompare(&_CmdHeader, prev_cmd) == 0 && prev_cmd->UserCallback == NULL)
    {
        CmdBuffer.pop_back();
        return;
    }

    curr_cmd->TextureId = _CmdHeader.TextureId;
}

void ImDrawList::_OnChangedVtxOffset()
{
    // We don't need to compare curr_cmd->VtxOffset != _CmdHeader.VtxOffset because we know it'll be different at the time we call this.
    _VtxCurrentIdx = 0;
    ImDrawCmd* curr_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    //IM_ASSERT(curr_cmd->VtxOffset != _CmdHeader.VtxOffset); // See #3349
    if (curr_cmd->ElemCount != 0)
    {
        AddDrawCmd();
        return;
    }
    IM_ASSERT(curr_cmd->UserCallback == NULL);
    curr_cmd->VtxOffset = _CmdHeader.VtxOffset;
}

// Render-level scissoring. This is passed down to your render function but not used for CPU-side coarse clipping. Prefer using higher-level ImGui::PushClipRect() to affect logic (hit-testing and widget culling)
void ImDrawList::PushClipRect(ImVec2 cr_min, ImVec2 cr_max, bool intersect_with_current_clip_rect)
{
    ImVec4 cr(cr_min.x, cr_min.y, cr_max.x, cr_max.y);
    if (intersect_with_current_clip_rect)
    {
        ImVec4 current = _CmdHeader.ClipRect;
        if (cr.x < current.x) cr.x = current.x;
        if (cr.y < current.y) cr.y = current.y;
        if (cr.z > current.z) cr.z = current.z;
        if (cr.w > current.w) cr.w = current.w;
    }
    cr.z = ImMax(cr.x, cr.z);
    cr.w = ImMax(cr.y, cr.w);

    _ClipRectStack.push_back(cr);
    _CmdHeader.ClipRect = cr;
    _OnChangedClipRect();
}

void ImDrawList::PushClipRectFullScreen()
{
    PushClipRect(ImVec2(_Data->ClipRectFullscreen.x, _Data->ClipRectFullscreen.y), ImVec2(_Data->ClipRectFullscreen.z, _Data->ClipRectFullscreen.w));
}

void ImDrawList::PopClipRect()
{
    _ClipRectStack.pop_back();
    _CmdHeader.ClipRect = (_ClipRectStack.Size == 0) ? _Data->ClipRectFullscreen : _ClipRectStack.Data[_ClipRectStack.Size - 1];
    _OnChangedClipRect();
}

void ImDrawList::PushTextureID(ImTextureID texture_id)
{
    _TextureIdStack.push_back(texture_id);
    _CmdHeader.TextureId = texture_id;
    _OnChangedTextureID();
}

void ImDrawList::PopTextureID()
{
    _TextureIdStack.pop_back();
    _CmdHeader.TextureId = (_TextureIdStack.Size == 0) ? (ImTextureID)NULL : _TextureIdStack.Data[_TextureIdStack.Size - 1];
    _OnChangedTextureID();
}

// Reserve space for a number of vertices and indices.
// You must finish filling your reserved data before calling PrimReserve() again, as it may reallocate or
// submit the intermediate results. PrimUnreserve() can be used to release unused allocations.
void ImDrawList::PrimReserve(int idx_count, int vtx_count)
{
    // Large mesh support (when enabled)
    IM_ASSERT_PARANOID(idx_count >= 0 && vtx_count >= 0);
    if (sizeof(ImDrawIdx) == 2 && (_VtxCurrentIdx + vtx_count >= (1 << 16)) && (Flags & ImDrawListFlags_AllowVtxOffset))
    {
        // FIXME: In theory we should be testing that vtx_count <64k here.
        // In practice, RenderText() relies on reserving ahead for a worst case scenario so it is currently useful for us
        // to not make that check until we rework the text functions to handle clipping and large horizontal lines better.
        _CmdHeader.VtxOffset = VtxBuffer.Size;
        _OnChangedVtxOffset();
    }

    ImDrawCmd* draw_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    draw_cmd->ElemCount += idx_count;

    int vtx_buffer_old_size = VtxBuffer.Size;
    VtxBuffer.resize(vtx_buffer_old_size + vtx_count);
    _VtxWritePtr = VtxBuffer.Data + vtx_buffer_old_size;

    int idx_buffer_old_size = IdxBuffer.Size;
    IdxBuffer.resize(idx_buffer_old_size + idx_count);
    _IdxWritePtr = IdxBuffer.Data + idx_buffer_old_size;
}

// Release the a number of reserved vertices/indices from the end of the last reservation made with PrimReserve().
void ImDrawList::PrimUnreserve(int idx_count, int vtx_count)
{
    IM_ASSERT_PARANOID(idx_count >= 0 && vtx_count >= 0);

    ImDrawCmd* draw_cmd = &CmdBuffer.Data[CmdBuffer.Size - 1];
    draw_cmd->ElemCount -= idx_count;
    VtxBuffer.shrink(VtxBuffer.Size - vtx_count);
    IdxBuffer.shrink(IdxBuffer.Size - idx_count);
}

// Fully unrolled with inline call to keep our debug builds decently fast.
void ImDrawList::PrimRect(const ImVec2& a, const ImVec2& c, ImU32 col)
{
    ImVec2 b(c.x, a.y), d(a.x, c.y), uv(_Data->TexUvWhitePixel);
    ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
    _IdxWritePtr[0] = idx; _IdxWritePtr[1] = (ImDrawIdx)(idx+1); _IdxWritePtr[2] = (ImDrawIdx)(idx+2);
    _IdxWritePtr[3] = idx; _IdxWritePtr[4] = (ImDrawIdx)(idx+2); _IdxWritePtr[5] = (ImDrawIdx)(idx+3);
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col;
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv; _VtxWritePtr[2].col = col;
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

void ImDrawList::PrimRectUV(const ImVec2& a, const ImVec2& c, const ImVec2& uv_a, const ImVec2& uv_c, ImU32 col)
{
    ImVec2 b(c.x, a.y), d(a.x, c.y), uv_b(uv_c.x, uv_a.y), uv_d(uv_a.x, uv_c.y);
    ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
    _IdxWritePtr[0] = idx; _IdxWritePtr[1] = (ImDrawIdx)(idx+1); _IdxWritePtr[2] = (ImDrawIdx)(idx+2);
    _IdxWritePtr[3] = idx; _IdxWritePtr[4] = (ImDrawIdx)(idx+2); _IdxWritePtr[5] = (ImDrawIdx)(idx+3);
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv_a; _VtxWritePtr[0].col = col;
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv_b; _VtxWritePtr[1].col = col;
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv_c; _VtxWritePtr[2].col = col;
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv_d; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

void ImDrawList::PrimQuadUV(const ImVec2& a, const ImVec2& b, const ImVec2& c, const ImVec2& d, const ImVec2& uv_a, const ImVec2& uv_b, const ImVec2& uv_c, const ImVec2& uv_d, ImU32 col)
{
    ImDrawIdx idx = (ImDrawIdx)_VtxCurrentIdx;
    _IdxWritePtr[0] = idx; _IdxWritePtr[1] = (ImDrawIdx)(idx+1); _IdxWritePtr[2] = (ImDrawIdx)(idx+2);
    _IdxWritePtr[3] = idx; _IdxWritePtr[4] = (ImDrawIdx)(idx+2); _IdxWritePtr[5] = (ImDrawIdx)(idx+3);
    _VtxWritePtr[0].pos = a; _VtxWritePtr[0].uv = uv_a; _VtxWritePtr[0].col = col;
    _VtxWritePtr[1].pos = b; _VtxWritePtr[1].uv = uv_b; _VtxWritePtr[1].col = col;
    _VtxWritePtr[2].pos = c; _VtxWritePtr[2].uv = uv_c; _VtxWritePtr[2].col = col;
    _VtxWritePtr[3].pos = d; _VtxWritePtr[3].uv = uv_d; _VtxWritePtr[3].col = col;
    _VtxWritePtr += 4;
    _VtxCurrentIdx += 4;
    _IdxWritePtr += 6;
}

// On AddPolyline() and AddConvexPolyFilled() we intentionally avoid using ImVec2 and superfluous function calls to optimize debug/non-inlined builds.
// Those macros expects l-values.
#define IM_NORMALIZE2F_OVER_ZERO(VX,VY)     do { float d2 = VX*VX + VY*VY; if (d2 > 0.0f) { float inv_len = 1.0f / ImSqrt(d2); VX *= inv_len; VY *= inv_len; } } while (0)
#define IM_FIXNORMAL2F(VX,VY)               do { float d2 = VX*VX + VY*VY; if (d2 < 0.5f) d2 = 0.5f; float inv_lensq = 1.0f / d2; VX *= inv_lensq; VY *= inv_lensq; } while (0)

// TODO: Thickness anti-aliased lines cap are missing their AA fringe.
// We avoid using the ImVec2 math operators here to reduce cost to a minimum for debug/non-inlined builds.
void ImDrawList::AddPolyline(const ImVec2* points, const int points_count, ImU32 col, bool closed, float thickness)
{
    if (points_count < 2)
        return;

    const ImVec2 opaque_uv = _Data->TexUvWhitePixel;
    const int count = closed ? points_count : points_count - 1; // The number of line segments we need to draw
    const bool thick_line = (thickness > 1.0f);

    if (Flags & ImDrawListFlags_AntiAliasedLines)
    {
        // Anti-aliased stroke
        const float AA_SIZE = 1.0f;
        const ImU32 col_trans = col & ~IM_COL32_A_MASK;

        // Thicknesses <1.0 should behave like thickness 1.0
        thickness = ImMax(thickness, 1.0f);
        const int integer_thickness = (int)thickness;
        const float fractional_thickness = thickness - integer_thickness;

        // Do we want to draw this line using a texture?
        // - For now, only draw integer-width lines using textures to avoid issues with the way scaling occurs, could be improved.
        // - If AA_SIZE is not 1.0f we cannot use the texture path.
        const bool use_texture = (Flags & ImDrawListFlags_AntiAliasedLinesUseTex) && (integer_thickness < IM_DRAWLIST_TEX_LINES_WIDTH_MAX) && (fractional_thickness <= 0.00001f);

        // We should never hit this, because NewFrame() doesn't set ImDrawListFlags_AntiAliasedLinesUseTex unless ImFontAtlasFlags_NoBakedLines is off
        IM_ASSERT_PARANOID(!use_texture || !(_Data->Font->ContainerAtlas->Flags & ImFontAtlasFlags_NoBakedLines));

        const int idx_count = use_texture ? (count * 6) : (thick_line ? count * 18 : count * 12);
        const int vtx_count = use_texture ? (points_count * 2) : (thick_line ? points_count * 4 : points_count * 3);
        PrimReserve(idx_count, vtx_count);

        // Temporary buffer
        // The first <points_count> items are normals at each line point, then after that there are either 2 or 4 temp points for each line point
        ImVec2* temp_normals = (ImVec2*)alloca(points_count * ((use_texture || !thick_line) ? 3 : 5) * sizeof(ImVec2)); //-V630
        ImVec2* temp_points = temp_normals + points_count;

        // Calculate normals (tangents) for each line segment
        for (int i1 = 0; i1 < count; i1++)
        {
            const int i2 = (i1 + 1) == points_count ? 0 : i1 + 1;
            float dx = points[i2].x - points[i1].x;
            float dy = points[i2].y - points[i1].y;
            IM_NORMALIZE2F_OVER_ZERO(dx, dy);
            temp_normals[i1].x = dy;
            temp_normals[i1].y = -dx;
        }
        if (!closed)
            temp_normals[points_count - 1] = temp_normals[points_count - 2];

        // If we are drawing a one-pixel-wide line without a texture, or a textured line of any width, we only need 2 or 3 vertices per point
        if (use_texture || !thick_line)
        {
            // [PATH 1] Texture-based lines (thick or non-thick)
            // [PATH 2] Non texture-based lines (non-thick)

            // The width of the geometry we need to draw - this is essentially <thickness> pixels for the line itself, plus "one pixel" for AA.
            // - In the texture-based path, we don't use AA_SIZE here because the +1 is tied to the generated texture
            //   (see ImFontAtlasBuildRenderLinesTexData() function), and so alternate values won't work without changes to that code.
            // - In the non texture-based paths, we would allow AA_SIZE to potentially be != 1.0f with a patch (e.g. fringe_scale patch to
            //   allow scaling geometry while preserving one-screen-pixel AA fringe).
            const float half_draw_size = use_texture ? ((thickness * 0.5f) + 1) : AA_SIZE;

            // If line is not closed, the first and last points need to be generated differently as there are no normals to blend
            if (!closed)
            {
                temp_points[0] = points[0] + temp_normals[0] * half_draw_size;
                temp_points[1] = points[0] - temp_normals[0] * half_draw_size;
                temp_points[(points_count-1)*2+0] = points[points_count-1] + temp_normals[points_count-1] * half_draw_size;
                temp_points[(points_count-1)*2+1] = points[points_count-1] - temp_normals[points_count-1] * half_draw_size;
            }

            // Generate the indices to form a number of triangles for each line segment, and the vertices for the line edges
            // This takes points n and n+1 and writes into n+1, with the first point in a closed line being generated from the final one (as n+1 wraps)
            // FIXME-OPT: Merge the different loops, possibly remove the temporary buffer.
            unsigned int idx1 = _VtxCurrentIdx; // Vertex index for start of line segment
            for (int i1 = 0; i1 < count; i1++) // i1 is the first point of the line segment
            {
                const int i2 = (i1 + 1) == points_count ? 0 : i1 + 1; // i2 is the second point of the line segment
                const unsigned int idx2 = ((i1 + 1) == points_count) ? _VtxCurrentIdx : (idx1 + (use_texture ? 2 : 3)); // Vertex index for end of segment

                // Average normals
                float dm_x = (temp_normals[i1].x + temp_normals[i2].x) * 0.5f;
                float dm_y = (temp_normals[i1].y + temp_normals[i2].y) * 0.5f;
                IM_FIXNORMAL2F(dm_x, dm_y);
                dm_x *= half_draw_size; // dm_x, dm_y are offset to the outer edge of the AA area
                dm_y *= half_draw_size;

                // Add temporary vertexes for the outer edges
                ImVec2* out_vtx = &temp_points[i2 * 2];
                out_vtx[0].x = points[i2].x + dm_x;
                out_vtx[0].y = points[i2].y + dm_y;
                out_vtx[1].x = points[i2].x - dm_x;
                out_vtx[1].y = points[i2].y - dm_y;

                if (use_texture)
                {
                    // Add indices for two triangles
                    _IdxWritePtr[0] = (ImDrawIdx)(idx2 + 0); _IdxWritePtr[1] = (ImDrawIdx)(idx1 + 0); _IdxWritePtr[2] = (ImDrawIdx)(idx1 + 1); // Right tri
                    _IdxWritePtr[3] = (ImDrawIdx)(idx2 + 1); _IdxWritePtr[4] = (ImDrawIdx)(idx1 + 1); _IdxWritePtr[5] = (ImDrawIdx)(idx2 + 0); // Left tri
                    _IdxWritePtr += 6;
                }
                else
                {
                    // Add indexes for four triangles
                    _IdxWritePtr[0] = (ImDrawIdx)(idx2 + 0); _IdxWritePtr[1] = (ImDrawIdx)(idx1 + 0); _IdxWritePtr[2] = (ImDrawIdx)(idx1 + 2); // Right tri 1
                    _IdxWritePtr[3] = (ImDrawIdx)(idx1 + 2); _IdxWritePtr[4] = (ImDrawIdx)(idx2 + 2); _IdxWritePtr[5] = (ImDrawIdx)(idx2 + 0); // Right tri 2
                    _IdxWritePtr[6] = (ImDrawIdx)(idx2 + 1); _IdxWritePtr[7] = (ImDrawIdx)(idx1 + 1); _IdxWritePtr[8] = (ImDrawIdx)(idx1 + 0); // Left tri 1
                    _IdxWritePtr[9] = (ImDrawIdx)(idx1 + 0); _IdxWritePtr[10] = (ImDrawIdx)(idx2 + 0); _IdxWritePtr[11] = (ImDrawIdx)(idx2 + 1); // Left tri 2
                    _IdxWritePtr += 12;
                }

                idx1 = idx2;
            }

            // Add vertexes for each point on the line
            if (use_texture)
            {
                // If we're using textures we only need to emit the left/right edge vertices
                ImVec4 tex_uvs = _Data->TexUvLines[integer_thickness];
                if (fractional_thickness != 0.0f)
                {
                    const ImVec4 tex_uvs_1 = _Data->TexUvLines[integer_thickness + 1];
                    tex_uvs.x = tex_uvs.x + (tex_uvs_1.x - tex_uvs.x) * fractional_thickness; // inlined ImLerp()
                    tex_uvs.y = tex_uvs.y + (tex_uvs_1.y - tex_uvs.y) * fractional_thickness;
                    tex_uvs.z = tex_uvs.z + (tex_uvs_1.z - tex_uvs.z) * fractional_thickness;
                    tex_uvs.w = tex_uvs.w + (tex_uvs_1.w - tex_uvs.w) * fractional_thickness;
                }
                ImVec2 tex_uv0(tex_uvs.x, tex_uvs.y);
                ImVec2 tex_uv1(tex_uvs.z, tex_uvs.w);
                for (int i = 0; i < points_count; i++)
                {
                    _VtxWritePtr[0].pos = temp_points[i * 2 + 0]; _VtxWritePtr[0].uv = tex_uv0; _VtxWritePtr[0].col = col; // Left-side outer edge
                    _VtxWritePtr[1].pos = temp_points[i * 2 + 1]; _VtxWritePtr[1].uv = tex_uv1; _VtxWritePtr[1].col = col; // Right-side outer edge
                    _VtxWritePtr += 2;
                }
            }
            else
            {
                // If we're not using a texture, we need the center vertex as well
                for (int i = 0; i < points_count; i++)
                {
                    _VtxWritePtr[0].pos = points[i];              _VtxWritePtr[0].uv = opaque_uv; _VtxWritePtr[0].col = col;       // Center of line
                    _VtxWritePtr[1].pos = temp_points[i * 2 + 0]; _VtxWritePtr[1].uv = opaque_uv; _VtxWritePtr[1].col = col_trans; // Left-side outer edge
                    _VtxWritePtr[2].pos = temp_points[i * 2 + 1]; _VtxWritePtr[2].uv = opaque_uv; _VtxWritePtr[2].col = col_trans; // Right-side outer edge
                    _VtxWritePtr += 3;
                }
            }
        }
        else
        {
            // [PATH 2] Non texture-based lines (thick): we need to draw the solid line core and thus require four vertices per point
            const float half_inner_thickness = (thickness - AA_SIZE) * 0.5f;

            // If line is not closed, the first and last points need to be generated differently as there are no normals to blend
            if (!closed)
            {
                const int points_last = points_count - 1;
                temp_points[0] = points[0] + temp_normals[0] * (half_inner_thickness + AA_SIZE);
                temp_points[1] = points[0] + temp_normals[0] * (half_inner_thickness);
                temp_points[2] = points[0] - temp_normals[0] * (half_inner_thickness);
                temp_points[3] = points[0] - temp_normals[0] * (half_inner_thickness + AA_SIZE);
                temp_points[points_last * 4 + 0] = points[points_last] + temp_normals[points_last] * (half_inner_thickness + AA_SIZE);
                temp_points[points_last * 4 + 1] = points[points_last] + temp_normals[points_last] * (half_inner_thickness);
                temp_points[points_last * 4 + 2] = points[points_last] - temp_normals[points_last] * (half_inner_thickness);
                temp_points[points_last * 4 + 3] = points[points_last] - temp_normals[points_last] * (half_inner_thickness + AA_SIZE);
            }

            // Generate the indices to form a number of triangles for each line segment, and the vertices for the line edges
            // This takes points n and n+1 and writes into n+1, with the first point in a closed line being generated from the final one (as n+1 wraps)
            // FIXME-OPT: Merge the different loops, possibly remove the temporary buffer.
            unsigned int idx1 = _VtxCurrentIdx; // Vertex index for start of line segment
            for (int i1 = 0; i1 < count; i1++) // i1 is the first point of the line segment
            {
                const int i2 = (i1 + 1) == points_count ? 0 : (i1 + 1); // i2 is the second point of the line segment
                const unsigned int idx2 = (i1 + 1) == points_count ? _VtxCurrentIdx : (idx1 + 4); // Vertex index for end of segment

                // Average normals
                float dm_x = (temp_normals[i1].x + temp_normals[i2].x) * 0.5f;
                float dm_y = (temp_normals[i1].y + temp_normals[i2].y) * 0.5f;
                IM_FIXNORMAL2F(dm_x, dm_y);
                float dm_out_x = dm_x * (half_inner_thickness + AA_SIZE);
                float dm_out_y = dm_y * (half_inner_thickness + AA_SIZE);
                float dm_in_x = dm_x * half_inner_thickness;
                float dm_in_y = dm_y * half_inner_thickness;

                // Add temporary vertices
                ImVec2* out_vtx = &temp_points[i2 * 4];
                out_vtx[0].x = points[i2].x + dm_out_x;
                out_vtx[0].y = points[i2].y + dm_out_y;
                out_vtx[1].x = points[i2].x + dm_in_x;
                out_vtx[1].y = points[i2].y + dm_in_y;
                out_vtx[2].x = points[i2].x - dm_in_x;
                out_vtx[2].y = points[i2].y - dm_in_y;
                out_vtx[3].x = points[i2].x - dm_out_x;
                out_vtx[3].y = points[i2].y - dm_out_y;

                // Add indexes
                _IdxWritePtr[0]  = (ImDrawIdx)(idx2 + 1); _IdxWritePtr[1]  = (ImDrawIdx)(idx1 + 1); _IdxWritePtr[2]  = (ImDrawIdx)(idx1 + 2);
                _IdxWritePtr[3]  = (ImDrawIdx)(idx1 + 2); _IdxWritePtr[4]  = (ImDrawIdx)(idx2 + 2); _IdxWritePtr[5]  = (ImDrawIdx)(idx2 + 1);
                _IdxWritePtr[6]  = (ImDrawIdx)(idx2 + 1); _IdxWritePtr[7]  = (ImDrawIdx)(idx1 + 1); _IdxWritePtr[8]  = (ImDrawIdx)(idx1 + 0);
                _IdxWritePtr[9]  = (ImDrawIdx)(idx1 + 0); _IdxWritePtr[10] = (ImDrawIdx)(idx2 + 0); _IdxWritePtr[11] = (ImDrawIdx)(idx2 + 1);
                _IdxWritePtr[12] = (ImDrawIdx)(idx2 + 2); _IdxWritePtr[13] = (ImDrawIdx)(idx1 + 2); _IdxWritePtr[14] = (ImDrawIdx)(idx1 + 3);
                _IdxWritePtr[15] = (ImDrawIdx)(idx1 + 3); _IdxWritePtr[16] = (ImDrawIdx)(idx2 + 3); _IdxWritePtr[17] = (ImDrawIdx)(idx2 + 2);
                _IdxWritePtr += 18;

                idx1 = idx2;
            }

            // Add vertices
            for (int i = 0; i < points_count; i++)
            {
                _VtxWritePtr[0].pos = temp_points[i * 4 + 0]; _VtxWritePtr[0].uv = opaque_uv; _VtxWritePtr[0].col = col_trans;
                _VtxWritePtr[1].pos = temp_points[i * 4 + 1]; _VtxWritePtr[1].uv = opaque_uv; _VtxWritePtr[1].col = col;
                _VtxWritePtr[2].pos = temp_points[i * 4 + 2]; _VtxWritePtr[2].uv = opaque_uv; _VtxWritePtr[2].col = col;
                _VtxWritePtr[3].pos = temp_points[i * 4 + 3]; _VtxWritePtr[3].uv = opaque_uv; _VtxWritePtr[3].col = col_trans;
                _VtxWritePtr += 4;
            }
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
    else
    {
        // [PATH 4] Non texture-based, Non anti-aliased lines
        const int idx_count = count * 6;
        const int vtx_count = count * 4;    // FIXME-OPT: Not sharing edges
        PrimReserve(idx_count, vtx_count);

        for (int i1 = 0; i1 < count; i1++)
        {
            const int i2 = (i1 + 1) == points_count ? 0 : i1 + 1;
            const ImVec2& p1 = points[i1];
            const ImVec2& p2 = points[i2];

            float dx = p2.x - p1.x;
            float dy = p2.y - p1.y;
            IM_NORMALIZE2F_OVER_ZERO(dx, dy);
            dx *= (thickness * 0.5f);
            dy *= (thickness * 0.5f);

            _VtxWritePtr[0].pos.x = p1.x + dy; _VtxWritePtr[0].pos.y = p1.y - dx; _VtxWritePtr[0].uv = opaque_uv; _VtxWritePtr[0].col = col;
            _VtxWritePtr[1].pos.x = p2.x + dy; _VtxWritePtr[1].pos.y = p2.y - dx; _VtxWritePtr[1].uv = opaque_uv; _VtxWritePtr[1].col = col;
            _VtxWritePtr[2].pos.x = p2.x - dy; _VtxWritePtr[2].pos.y = p2.y + dx; _VtxWritePtr[2].uv = opaque_uv; _VtxWritePtr[2].col = col;
            _VtxWritePtr[3].pos.x = p1.x - dy; _VtxWritePtr[3].pos.y = p1.y + dx; _VtxWritePtr[3].uv = opaque_uv; _VtxWritePtr[3].col = col;
            _VtxWritePtr += 4;

            _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx + 1); _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx + 2);
            _IdxWritePtr[3] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[4] = (ImDrawIdx)(_VtxCurrentIdx + 2); _IdxWritePtr[5] = (ImDrawIdx)(_VtxCurrentIdx + 3);
            _IdxWritePtr += 6;
            _VtxCurrentIdx += 4;
        }
    }
}

// We intentionally avoid using ImVec2 and its math operators here to reduce cost to a minimum for debug/non-inlined builds.
void ImDrawList::AddConvexPolyFilled(const ImVec2* points, const int points_count, ImU32 col)
{
    if (points_count < 3)
        return;

    const ImVec2 uv = _Data->TexUvWhitePixel;

    if (Flags & ImDrawListFlags_AntiAliasedFill)
    {
        // Anti-aliased Fill
        const float AA_SIZE = 1.0f;
        const ImU32 col_trans = col & ~IM_COL32_A_MASK;
        const int idx_count = (points_count - 2)*3 + points_count * 6;
        const int vtx_count = (points_count * 2);
        PrimReserve(idx_count, vtx_count);

        // Add indexes for fill
        unsigned int vtx_inner_idx = _VtxCurrentIdx;
        unsigned int vtx_outer_idx = _VtxCurrentIdx + 1;
        for (int i = 2; i < points_count; i++)
        {
            _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx); _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx + ((i - 1) << 1)); _IdxWritePtr[2] = (ImDrawIdx)(vtx_inner_idx + (i << 1));
            _IdxWritePtr += 3;
        }

        // Compute normals
        ImVec2* temp_normals = (ImVec2*)alloca(points_count * sizeof(ImVec2)); //-V630
        for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            const ImVec2& p0 = points[i0];
            const ImVec2& p1 = points[i1];
            float dx = p1.x - p0.x;
            float dy = p1.y - p0.y;
            IM_NORMALIZE2F_OVER_ZERO(dx, dy);
            temp_normals[i0].x = dy;
            temp_normals[i0].y = -dx;
        }

        for (int i0 = points_count - 1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            // Average normals
            const ImVec2& n0 = temp_normals[i0];
            const ImVec2& n1 = temp_normals[i1];
            float dm_x = (n0.x + n1.x) * 0.5f;
            float dm_y = (n0.y + n1.y) * 0.5f;
            IM_FIXNORMAL2F(dm_x, dm_y);
            dm_x *= AA_SIZE * 0.5f;
            dm_y *= AA_SIZE * 0.5f;

            // Add vertices
            _VtxWritePtr[0].pos.x = (points[i1].x - dm_x); _VtxWritePtr[0].pos.y = (points[i1].y - dm_y); _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;        // Inner
            _VtxWritePtr[1].pos.x = (points[i1].x + dm_x); _VtxWritePtr[1].pos.y = (points[i1].y + dm_y); _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col_trans;  // Outer
            _VtxWritePtr += 2;

            // Add indexes for fringes
            _IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx + (i1 << 1)); _IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx + (i0 << 1)); _IdxWritePtr[2] = (ImDrawIdx)(vtx_outer_idx + (i0 << 1));
            _IdxWritePtr[3] = (ImDrawIdx)(vtx_outer_idx + (i0 << 1)); _IdxWritePtr[4] = (ImDrawIdx)(vtx_outer_idx + (i1 << 1)); _IdxWritePtr[5] = (ImDrawIdx)(vtx_inner_idx + (i1 << 1));
            _IdxWritePtr += 6;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
    else
    {
        // Non Anti-aliased Fill
        const int idx_count = (points_count - 2)*3;
        const int vtx_count = points_count;
        PrimReserve(idx_count, vtx_count);
        for (int i = 0; i < vtx_count; i++)
        {
            _VtxWritePtr[0].pos = points[i]; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
            _VtxWritePtr++;
        }
        for (int i = 2; i < points_count; i++)
        {
            _IdxWritePtr[0] = (ImDrawIdx)(_VtxCurrentIdx); _IdxWritePtr[1] = (ImDrawIdx)(_VtxCurrentIdx + i - 1); _IdxWritePtr[2] = (ImDrawIdx)(_VtxCurrentIdx + i);
            _IdxWritePtr += 3;
        }
        _VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
}

void ImDrawList::PathArcToFast(const ImVec2& center, float radius, int a_min_of_12, int a_max_of_12)
{
    if (radius == 0.0f || a_min_of_12 > a_max_of_12)
    {
        _Path.push_back(center);
        return;
    }

    // For legacy reason the PathArcToFast() always takes angles where 2*PI is represented by 12,
    // but it is possible to set IM_DRAWLIST_ARCFAST_TESSELATION_MULTIPLIER to a higher value. This should compile to a no-op otherwise.
#if IM_DRAWLIST_ARCFAST_TESSELLATION_MULTIPLIER != 1
    a_min_of_12 *= IM_DRAWLIST_ARCFAST_TESSELLATION_MULTIPLIER;
    a_max_of_12 *= IM_DRAWLIST_ARCFAST_TESSELLATION_MULTIPLIER;
#endif

    _Path.reserve(_Path.Size + (a_max_of_12 - a_min_of_12 + 1));
    for (int a = a_min_of_12; a <= a_max_of_12; a++)
    {
        const ImVec2& c = _Data->ArcFastVtx[a % IM_ARRAYSIZE(_Data->ArcFastVtx)];
        _Path.push_back(ImVec2(center.x + c.x * radius, center.y + c.y * radius));
    }
}

void ImDrawList::PathEllipticalArcTo(const ImVec2& center, float radius_x, float radius_y, float rot, float a_min, float a_max, int num_segments)
{
    _Path.reserve(_Path.Size + (num_segments + 1));

    const float cosRot = ImCos(rot);
    const float sinRot = ImSin(rot);
    for (int i = 0; i <= num_segments; i++)
    {
        const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        ImVec2 point(center.x + ImCos(a) * radius_x, center.y + ImSin(a) * radius_y);
        point.x -= center.x;
        point.y -= center.y;
        const float rel_x = (point.x * cosRot) - (point.y * sinRot);
        const float rel_y = (point.x * sinRot) + (point.y * cosRot);
        point.x = rel_x + center.x;
        point.y = rel_y + center.y;
        _Path.push_back(point);
    }
}

void ImDrawList::PathArcTo(const ImVec2& center, float radius, float a_min, float a_max, int num_segments)
{
    if (radius == 0.0f)
    {
        _Path.push_back(center);
        return;
    }

    // Note that we are adding a point at both a_min and a_max.
    // If you are trying to draw a full closed circle you don't want the overlapping points!
    _Path.reserve(_Path.Size + (num_segments + 1));
    for (int i = 0; i <= num_segments; i++)
    {
        const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        _Path.push_back(ImVec2(center.x + ImCos(a) * radius, center.y + ImSin(a) * radius));
    }
}

ImVec2 ImBezierCalc(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, float t)
{
    float u = 1.0f - t;
    float w1 = u*u*u;
    float w2 = 3*u*u*t;
    float w3 = 3*u*t*t;
    float w4 = t*t*t;
    return ImVec2(w1*p1.x + w2*p2.x + w3*p3.x + w4*p4.x, w1*p1.y + w2*p2.y + w3*p3.y + w4*p4.y);
}

// Closely mimics BezierClosestPointCasteljauStep() in imgui.cpp
static void PathBezierToCasteljau(ImVector<ImVec2>* path, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float tess_tol, int level)
{
    float dx = x4 - x1;
    float dy = y4 - y1;
    float d2 = ((x2 - x4) * dy - (y2 - y4) * dx);
    float d3 = ((x3 - x4) * dy - (y3 - y4) * dx);
    d2 = (d2 >= 0) ? d2 : -d2;
    d3 = (d3 >= 0) ? d3 : -d3;
    if ((d2 + d3) * (d2 + d3) < tess_tol * (dx * dx + dy * dy))
    {
        path->push_back(ImVec2(x4, y4));
    }
    else if (level < 10)
    {
        float x12 = (x1 + x2)*0.5f,       y12 = (y1 + y2)*0.5f;
        float x23 = (x2 + x3)*0.5f,       y23 = (y2 + y3)*0.5f;
        float x34 = (x3 + x4)*0.5f,       y34 = (y3 + y4)*0.5f;
        float x123 = (x12 + x23)*0.5f,    y123 = (y12 + y23)*0.5f;
        float x234 = (x23 + x34)*0.5f,    y234 = (y23 + y34)*0.5f;
        float x1234 = (x123 + x234)*0.5f, y1234 = (y123 + y234)*0.5f;
        PathBezierToCasteljau(path, x1, y1,        x12, y12,    x123, y123,  x1234, y1234, tess_tol, level + 1);
        PathBezierToCasteljau(path, x1234, y1234,  x234, y234,  x34, y34,    x4, y4,       tess_tol, level + 1);
    }
}

void ImDrawList::PathBezierCurveTo(const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, int num_segments)
{
    ImVec2 p1 = _Path.back();
    if (num_segments == 0)
    {
        PathBezierToCasteljau(&_Path, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y, _Data->CurveTessellationTol, 0); // Auto-tessellated
    }
    else
    {
        float t_step = 1.0f / (float)num_segments;
        for (int i_step = 1; i_step <= num_segments; i_step++)
            _Path.push_back(ImBezierCalc(p1, p2, p3, p4, t_step * i_step));
    }
}

void ImDrawList::PathRect(const ImVec2& a, const ImVec2& b, float rounding, ImDrawCornerFlags rounding_corners)
{
    rounding = ImMin(rounding, ImFabs(b.x - a.x) * ( ((rounding_corners & ImDrawCornerFlags_Top)  == ImDrawCornerFlags_Top)  || ((rounding_corners & ImDrawCornerFlags_Bot)   == ImDrawCornerFlags_Bot)   ? 0.5f : 1.0f ) - 1.0f);
    rounding = ImMin(rounding, ImFabs(b.y - a.y) * ( ((rounding_corners & ImDrawCornerFlags_Left) == ImDrawCornerFlags_Left) || ((rounding_corners & ImDrawCornerFlags_Right) == ImDrawCornerFlags_Right) ? 0.5f : 1.0f ) - 1.0f);

    if (rounding <= 0.0f || rounding_corners == 0)
    {
        PathLineTo(a);
        PathLineTo(ImVec2(b.x, a.y));
        PathLineTo(b);
        PathLineTo(ImVec2(a.x, b.y));
    }
    else
    {
        const float rounding_tl = (rounding_corners & ImDrawCornerFlags_TopLeft) ? rounding : 0.0f;
        const float rounding_tr = (rounding_corners & ImDrawCornerFlags_TopRight) ? rounding : 0.0f;
        const float rounding_br = (rounding_corners & ImDrawCornerFlags_BotRight) ? rounding : 0.0f;
        const float rounding_bl = (rounding_corners & ImDrawCornerFlags_BotLeft) ? rounding : 0.0f;
        PathArcToFast(ImVec2(a.x + rounding_tl, a.y + rounding_tl), rounding_tl, 6, 9);
        PathArcToFast(ImVec2(b.x - rounding_tr, a.y + rounding_tr), rounding_tr, 9, 12);
        PathArcToFast(ImVec2(b.x - rounding_br, b.y - rounding_br), rounding_br, 0, 3);
        PathArcToFast(ImVec2(a.x + rounding_bl, b.y - rounding_bl), rounding_bl, 3, 6);
    }
}

void ImDrawList::AddLine(const ImVec2& p1, const ImVec2& p2, ImU32 col, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;
    PathLineTo(p1 + ImVec2(0.5f, 0.5f));
    PathLineTo(p2 + ImVec2(0.5f, 0.5f));
    PathStroke(col, false, thickness);
}

// p_min = upper-left, p_max = lower-right
// Note we don't render 1 pixels sized rectangles properly.
void ImDrawList::AddRect(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding, ImDrawCornerFlags rounding_corners, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;
    if (Flags & ImDrawListFlags_AntiAliasedLines)
        PathRect(p_min + ImVec2(0.50f, 0.50f), p_max - ImVec2(0.50f, 0.50f), rounding, rounding_corners);
    else
        PathRect(p_min + ImVec2(0.50f, 0.50f), p_max - ImVec2(0.49f, 0.49f), rounding, rounding_corners); // Better looking lower-right corner and rounded non-AA shapes.
    PathStroke(col, true, thickness);
}

void ImDrawList::AddRectFilled(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding, ImDrawCornerFlags rounding_corners)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;
    if (rounding > 0.0f)
    {
        PathRect(p_min, p_max, rounding, rounding_corners);
        PathFillConvex(col);
    }
    else
    {
        PrimReserve(6, 4);
        PrimRect(p_min, p_max, col);
    }
}

// p_min = upper-left, p_max = lower-right
void ImDrawList::AddRectFilledMultiColor(const ImVec2& p_min, const ImVec2& p_max, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left)
{
    if (((col_upr_left | col_upr_right | col_bot_right | col_bot_left) & IM_COL32_A_MASK) == 0)
        return;

    const ImVec2 uv = _Data->TexUvWhitePixel;
    PrimReserve(6, 4);
    PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx + 1)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx + 2));
    PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx + 2)); PrimWriteIdx((ImDrawIdx)(_VtxCurrentIdx + 3));
    PrimWriteVtx(p_min, uv, col_upr_left);
    PrimWriteVtx(ImVec2(p_max.x, p_min.y), uv, col_upr_right);
    PrimWriteVtx(p_max, uv, col_bot_right);
    PrimWriteVtx(ImVec2(p_min.x, p_max.y), uv, col_bot_left);
}

void ImDrawList::AddQuad(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 col, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathLineTo(p2);
    PathLineTo(p3);
    PathLineTo(p4);
    PathStroke(col, true, thickness);
}

void ImDrawList::AddQuadFilled(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathLineTo(p2);
    PathLineTo(p3);
    PathLineTo(p4);
    PathFillConvex(col);
}

void ImDrawList::AddTriangle(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathLineTo(p2);
    PathLineTo(p3);
    PathStroke(col, true, thickness);
}

void ImDrawList::AddTriangleFilled(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathLineTo(p2);
    PathLineTo(p3);
    PathFillConvex(col);
}

void ImDrawList::AddCircle(const ImVec2& center, float radius, ImU32 col, int num_segments, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0 || radius <= 0.0f)
        return;

    // Obtain segment count
    if (num_segments <= 0)
    {
        // Automatic segment count
        const int radius_idx = (int)radius - 1;
        if (radius_idx < IM_ARRAYSIZE(_Data->CircleSegmentCounts))
            num_segments = _Data->CircleSegmentCounts[radius_idx]; // Use cached value
        else
            num_segments = IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC(radius, _Data->CircleSegmentMaxError);
    }
    else
    {
        // Explicit segment count (still clamp to avoid drawing insanely tessellated shapes)
        num_segments = ImClamp(num_segments, 3, IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_MAX);
    }

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = (IM_PI * 2.0f) * ((float)num_segments - 1.0f) / (float)num_segments;
    if (num_segments == 12)
        PathArcToFast(center, radius - 0.5f, 0, 12 - 1);
    else
        PathArcTo(center, radius - 0.5f, 0.0f, a_max, num_segments - 1);
    PathStroke(col, true, thickness);
}

void ImDrawList::AddCircleFilled(const ImVec2& center, float radius, ImU32 col, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0 || radius <= 0.0f)
        return;

    // Obtain segment count
    if (num_segments <= 0)
    {
        // Automatic segment count
        const int radius_idx = (int)radius - 1;
        if (radius_idx < IM_ARRAYSIZE(_Data->CircleSegmentCounts))
            num_segments = _Data->CircleSegmentCounts[radius_idx]; // Use cached value
        else
            num_segments = IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_CALC(radius, _Data->CircleSegmentMaxError);
    }
    else
    {
        // Explicit segment count (still clamp to avoid drawing insanely tessellated shapes)
        num_segments = ImClamp(num_segments, 3, IM_DRAWLIST_CIRCLE_AUTO_SEGMENT_MAX);
    }

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = (IM_PI * 2.0f) * ((float)num_segments - 1.0f) / (float)num_segments;
    if (num_segments == 12)
        PathArcToFast(center, radius, 0, 12 - 1);
    else
        PathArcTo(center, radius, 0.0f, a_max, num_segments - 1);
    PathFillConvex(col);
}

// Guaranteed to honor 'num_segments'
void ImDrawList::AddNgon(const ImVec2& center, float radius, ImU32 col, int num_segments, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0 || num_segments <= 2)
        return;

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = (IM_PI * 2.0f) * ((float)num_segments - 1.0f) / (float)num_segments;
    PathArcTo(center, radius - 0.5f, 0.0f, a_max, num_segments - 1);
    PathStroke(col, true, thickness);
}

// Guaranteed to honor 'num_segments'
void ImDrawList::AddNgonFilled(const ImVec2& center, float radius, ImU32 col, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0 || num_segments <= 2)
        return;

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = (IM_PI * 2.0f) * ((float)num_segments - 1.0f) / (float)num_segments;
    PathArcTo(center, radius, 0.0f, a_max, num_segments - 1);
    PathFillConvex(col);
}

void ImDrawList::AddEllipse(const ImVec2& center, float radius_x, float radius_y, ImU32 col, float rot, int num_segments, float thickness)
{
    if ((col & IM_COL32_A_MASK) == 0 || num_segments <= 2)
        return;

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = IM_PI * 2.0f * ((float)num_segments - 1.0f) / (float)num_segments;
    PathEllipticalArcTo(center, radius_x, radius_y, rot, 0.0f, a_max, num_segments - 1);
    PathStroke(col, true, thickness);
}

void ImDrawList::AddEllipseFilled(const ImVec2& center, float radius_x, float radius_y, ImU32 col, float rot, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0 || num_segments <= 2)
        return;

    // Because we are filling a closed shape we remove 1 from the count of segments/points
    const float a_max = IM_PI * 2.0f * ((float)num_segments - 1.0f) / (float)num_segments;
    PathEllipticalArcTo(center, radius_x, radius_y, rot, 0.0f, a_max, num_segments - 1);
    PathFillConvex(col);
}

// Cubic Bezier takes 4 controls points
void ImDrawList::AddBezierCurve(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 col, float thickness, int num_segments)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    PathLineTo(p1);
    PathBezierCurveTo(p2, p3, p4, num_segments);
    PathStroke(col, false, thickness);
}

void ImDrawList::AddText(const ImFont* font, float font_size, const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end, float wrap_width, const ImVec4* cpu_fine_clip_rect)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    if (text_end == NULL)
        text_end = text_begin + strlen(text_begin);
    if (text_begin == text_end)
        return;

    // Pull default font/size from the shared ImDrawListSharedData instance
    if (font == NULL)
        font = _Data->Font;
    if (font_size == 0.0f)
        font_size = _Data->FontSize;

    IM_ASSERT(font->ContainerAtlas->TexID == _CmdHeader.TextureId);  // Use high-level ImGui::PushFont() or low-level ImDrawList::PushTextureId() to change font.

    ImVec4 clip_rect = _CmdHeader.ClipRect;
    if (cpu_fine_clip_rect)
    {
        clip_rect.x = ImMax(clip_rect.x, cpu_fine_clip_rect->x);
        clip_rect.y = ImMax(clip_rect.y, cpu_fine_clip_rect->y);
        clip_rect.z = ImMin(clip_rect.z, cpu_fine_clip_rect->z);
        clip_rect.w = ImMin(clip_rect.w, cpu_fine_clip_rect->w);
    }
    font->RenderText(this, font_size, pos, col, clip_rect, text_begin, text_end, wrap_width, cpu_fine_clip_rect != NULL);
}

void ImDrawList::AddText(const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end)
{
    AddText(NULL, 0.0f, pos, col, text_begin, text_end);
}

void ImDrawList::AddImage(ImTextureID user_texture_id, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min, const ImVec2& uv_max, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    const bool push_texture_id = user_texture_id != _CmdHeader.TextureId;
    if (push_texture_id)
        PushTextureID(user_texture_id);

    PrimReserve(6, 4);
    PrimRectUV(p_min, p_max, uv_min, uv_max, col);

    if (push_texture_id)
        PopTextureID();
}

void ImDrawList::AddImageQuad(ImTextureID user_texture_id, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, const ImVec2& uv1, const ImVec2& uv2, const ImVec2& uv3, const ImVec2& uv4, ImU32 col)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    const bool push_texture_id = user_texture_id != _CmdHeader.TextureId;
    if (push_texture_id)
        PushTextureID(user_texture_id);

    PrimReserve(6, 4);
    PrimQuadUV(p1, p2, p3, p4, uv1, uv2, uv3, uv4, col);

    if (push_texture_id)
        PopTextureID();
}

void ImDrawList::AddImageRounded(ImTextureID user_texture_id, const ImVec2& p_min, const ImVec2& p_max, const ImVec2& uv_min, const ImVec2& uv_max, ImU32 col, float rounding, ImDrawCornerFlags rounding_corners)
{
    if ((col & IM_COL32_A_MASK) == 0)
        return;

    if (rounding <= 0.0f || (rounding_corners & ImDrawCornerFlags_All) == 0)
    {
        AddImage(user_texture_id, p_min, p_max, uv_min, uv_max, col);
        return;
    }

    const bool push_texture_id = _TextureIdStack.empty() || user_texture_id != _TextureIdStack.back();
    if (push_texture_id)
        PushTextureID(user_texture_id);

    int vert_start_idx = VtxBuffer.Size;
    PathRect(p_min, p_max, rounding, rounding_corners);
    PathFillConvex(col);
    int vert_end_idx = VtxBuffer.Size;
    ImGui::ShadeVertsLinearUV(this, vert_start_idx, vert_end_idx, p_min, p_max, uv_min, uv_max, true);

    if (push_texture_id)
        PopTextureID();
}


//-----------------------------------------------------------------------------
// ImDrawListSplitter
//-----------------------------------------------------------------------------
// FIXME: This may be a little confusing, trying to be a little too low-level/optimal instead of just doing vector swap..
//-----------------------------------------------------------------------------

void ImDrawListSplitter::ClearFreeMemory()
{
    for (int i = 0; i < _Channels.Size; i++)
    {
        if (i == _Current)
            memset(&_Channels[i], 0, sizeof(_Channels[i]));  // Current channel is a copy of CmdBuffer/IdxBuffer, don't destruct again
        _Channels[i]._CmdBuffer.clear();
        _Channels[i]._IdxBuffer.clear();
    }
    _Current = 0;
    _Count = 1;
    _Channels.clear();
}

void ImDrawListSplitter::Split(ImDrawList* draw_list, int channels_count)
{
    IM_ASSERT(_Current == 0 && _Count <= 1 && "Nested channel splitting is not supported. Please use separate instances of ImDrawListSplitter.");
    int old_channels_count = _Channels.Size;
    if (old_channels_count < channels_count)
        _Channels.resize(channels_count);
    _Count = channels_count;

    // Channels[] (24/32 bytes each) hold storage that we'll swap with draw_list->_CmdBuffer/_IdxBuffer
    // The content of Channels[0] at this point doesn't matter. We clear it to make state tidy in a debugger but we don't strictly need to.
    // When we switch to the next channel, we'll copy draw_list->_CmdBuffer/_IdxBuffer into Channels[0] and then Channels[1] into draw_list->CmdBuffer/_IdxBuffer
    memset(&_Channels[0], 0, sizeof(ImDrawChannel));
    for (int i = 1; i < channels_count; i++)
    {
        if (i >= old_channels_count)
        {
            IM_PLACEMENT_NEW(&_Channels[i]) ImDrawChannel();
        }
        else
        {
            _Channels[i]._CmdBuffer.resize(0);
            _Channels[i]._IdxBuffer.resize(0);
        }
        if (_Channels[i]._CmdBuffer.Size == 0)
        {
            ImDrawCmd draw_cmd;
            ImDrawCmd_HeaderCopy(&draw_cmd, &draw_list->_CmdHeader); // Copy ClipRect, TextureId, VtxOffset
            _Channels[i]._CmdBuffer.push_back(draw_cmd);
        }
    }
}

void ImDrawListSplitter::Merge(ImDrawList* draw_list)
{
    // Note that we never use or rely on _Channels.Size because it is merely a buffer that we never shrink back to 0 to keep all sub-buffers ready for use.
    if (_Count <= 1)
        return;

    SetCurrentChannel(draw_list, 0);
    draw_list->_PopUnusedDrawCmd();

    // Calculate our final buffer sizes. Also fix the incorrect IdxOffset values in each command.
    int new_cmd_buffer_count = 0;
    int new_idx_buffer_count = 0;
    ImDrawCmd* last_cmd = (_Count > 0 && draw_list->CmdBuffer.Size > 0) ? &draw_list->CmdBuffer.back() : NULL;
    int idx_offset = last_cmd ? last_cmd->IdxOffset + last_cmd->ElemCount : 0;
    for (int i = 1; i < _Count; i++)
    {
        ImDrawChannel& ch = _Channels[i];

        // Equivalent of PopUnusedDrawCmd() for this channel's cmdbuffer and except we don't need to test for UserCallback.
        if (ch._CmdBuffer.Size > 0 && ch._CmdBuffer.back().ElemCount == 0)
            ch._CmdBuffer.pop_back();

        if (ch._CmdBuffer.Size > 0 && last_cmd != NULL)
        {
            ImDrawCmd* next_cmd = &ch._CmdBuffer[0];
            if (ImDrawCmd_HeaderCompare(last_cmd, next_cmd) == 0 && last_cmd->UserCallback == NULL && next_cmd->UserCallback == NULL)
            {
                // Merge previous channel last draw command with current channel first draw command if matching.
                last_cmd->ElemCount += next_cmd->ElemCount;
                idx_offset += next_cmd->ElemCount;
                ch._CmdBuffer.erase(ch._CmdBuffer.Data); // FIXME-OPT: Improve for multiple merges.
            }
        }
        if (ch._CmdBuffer.Size > 0)
            last_cmd = &ch._CmdBuffer.back();
        new_cmd_buffer_count += ch._CmdBuffer.Size;
        new_idx_buffer_count += ch._IdxBuffer.Size;
        for (int cmd_n = 0; cmd_n < ch._CmdBuffer.Size; cmd_n++)
        {
            ch._CmdBuffer.Data[cmd_n].IdxOffset = idx_offset;
            idx_offset += ch._CmdBuffer.Data[cmd_n].ElemCount;
        }
    }
    draw_list->CmdBuffer.resize(draw_list->CmdBuffer.Size + new_cmd_buffer_count);
    draw_list->IdxBuffer.resize(draw_list->IdxBuffer.Size + new_idx_buffer_count);

    // Write commands and indices in order (they are fairly small structures, we don't copy vertices only indices)
    ImDrawCmd* cmd_write = draw_list->CmdBuffer.Data + draw_list->CmdBuffer.Size - new_cmd_buffer_count;
    ImDrawIdx* idx_write = draw_list->IdxBuffer.Data + draw_list->IdxBuffer.Size - new_idx_buffer_count;
    for (int i = 1; i < _Count; i++)
    {
        ImDrawChannel& ch = _Channels[i];
        if (int sz = ch._CmdBuffer.Size) { memcpy(cmd_write, ch._CmdBuffer.Data, sz * sizeof(ImDrawCmd)); cmd_write += sz; }
        if (int sz = ch._IdxBuffer.Size) { memcpy(idx_write, ch._IdxBuffer.Data, sz * sizeof(ImDrawIdx)); idx_write += sz; }
    }
    draw_list->_IdxWritePtr = idx_write;

    // Ensure there's always a non-callback draw command trailing the command-buffer
    if (draw_list->CmdBuffer.Size == 0 || draw_list->CmdBuffer.back().UserCallback != NULL)
        draw_list->AddDrawCmd();

    // If current command is used with different settings we need to add a new command
    ImDrawCmd* curr_cmd = &draw_list->CmdBuffer.Data[draw_list->CmdBuffer.Size - 1];
    if (curr_cmd->ElemCount == 0)
        ImDrawCmd_HeaderCopy(curr_cmd, &draw_list->_CmdHeader); // Copy ClipRect, TextureId, VtxOffset
    else if (ImDrawCmd_HeaderCompare(curr_cmd, &draw_list->_CmdHeader) != 0)
        draw_list->AddDrawCmd();

    _Count = 1;
}

void ImDrawListSplitter::SetCurrentChannel(ImDrawList* draw_list, int idx)
{
    IM_ASSERT(idx >= 0 && idx < _Count);
    if (_Current == idx)
        return;

    // Overwrite ImVector (12/16 bytes), four times. This is merely a silly optimization instead of doing .swap()
    memcpy(&_Channels.Data[_Current]._CmdBuffer, &draw_list->CmdBuffer, sizeof(draw_list->CmdBuffer));
    memcpy(&_Channels.Data[_Current]._IdxBuffer, &draw_list->IdxBuffer, sizeof(draw_list->IdxBuffer));
    _Current = idx;
    memcpy(&draw_list->CmdBuffer, &_Channels.Data[idx]._CmdBuffer, sizeof(draw_list->CmdBuffer));
    memcpy(&draw_list->IdxBuffer, &_Channels.Data[idx]._IdxBuffer, sizeof(draw_list->IdxBuffer));
    draw_list->_IdxWritePtr = draw_list->IdxBuffer.Data + draw_list->IdxBuffer.Size;

    // If current command is used with different settings we need to add a new command
    ImDrawCmd* curr_cmd = &draw_list->CmdBuffer.Data[draw_list->CmdBuffer.Size - 1];
    if (curr_cmd->ElemCount == 0)
        ImDrawCmd_HeaderCopy(curr_cmd, &draw_list->_CmdHeader); // Copy ClipRect, TextureId, VtxOffset
    else if (ImDrawCmd_HeaderCompare(curr_cmd, &draw_list->_CmdHeader) != 0)
        draw_list->AddDrawCmd();
}

//-----------------------------------------------------------------------------
// [SECTION] ImDrawData
//-----------------------------------------------------------------------------

// For backward compatibility: convert all buffers from indexed to de-indexed, in case you cannot render indexed. Note: this is slow and most likely a waste of resources. Always prefer indexed rendering!
void ImDrawData::DeIndexAllBuffers()
{
    ImVector<ImDrawVert> new_vtx_buffer;
    TotalVtxCount = TotalIdxCount = 0;
    for (int i = 0; i < CmdListsCount; i++)
    {
        ImDrawList* cmd_list = CmdLists[i];
        if (cmd_list->IdxBuffer.empty())
            continue;
        new_vtx_buffer.resize(cmd_list->IdxBuffer.Size);
        for (int j = 0; j < cmd_list->IdxBuffer.Size; j++)
            new_vtx_buffer[j] = cmd_list->VtxBuffer[cmd_list->IdxBuffer[j]];
        cmd_list->VtxBuffer.swap(new_vtx_buffer);
        cmd_list->IdxBuffer.resize(0);
        TotalVtxCount += cmd_list->VtxBuffer.Size;
    }
}

// Helper to scale the ClipRect field of each ImDrawCmd.
// Use if your final output buffer is at a different scale than draw_data->DisplaySize,
// or if there is a difference between your window resolution and framebuffer resolution.
void ImDrawData::ScaleClipRects(const ImVec2& fb_scale)
{
    for (int i = 0; i < CmdListsCount; i++)
    {
        ImDrawList* cmd_list = CmdLists[i];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            ImDrawCmd* cmd = &cmd_list->CmdBuffer[cmd_i];
            cmd->ClipRect = ImVec4(cmd->ClipRect.x * fb_scale.x, cmd->ClipRect.y * fb_scale.y, cmd->ClipRect.z * fb_scale.x, cmd->ClipRect.w * fb_scale.y);
        }
    }
}

//-----------------------------------------------------------------------------
// [SECTION] Helpers ShadeVertsXXX functions
//-----------------------------------------------------------------------------

// Generic linear color gradient, write to RGB fields, leave A untouched.
void ImGui::ShadeVertsLinearColorGradientKeepAlpha(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, ImVec2 gradient_p0, ImVec2 gradient_p1, ImU32 col0, ImU32 col1)
{
    ImVec2 gradient_extent = gradient_p1 - gradient_p0;
    float gradient_inv_length2 = 1.0f / ImLengthSqr(gradient_extent);
    ImDrawVert* vert_start = draw_list->VtxBuffer.Data + vert_start_idx;
    ImDrawVert* vert_end = draw_list->VtxBuffer.Data + vert_end_idx;
    const int col0_r = (int)(col0 >> IM_COL32_R_SHIFT) & 0xFF;
    const int col0_g = (int)(col0 >> IM_COL32_G_SHIFT) & 0xFF;
    const int col0_b = (int)(col0 >> IM_COL32_B_SHIFT) & 0xFF;
    const int col_delta_r = ((int)(col1 >> IM_COL32_R_SHIFT) & 0xFF) - col0_r;
    const int col_delta_g = ((int)(col1 >> IM_COL32_G_SHIFT) & 0xFF) - col0_g;
    const int col_delta_b = ((int)(col1 >> IM_COL32_B_SHIFT) & 0xFF) - col0_b;
    for (ImDrawVert* vert = vert_start; vert < vert_end; vert++)
    {
        float d = ImDot(vert->pos - gradient_p0, gradient_extent);
        float t = ImClamp(d * gradient_inv_length2, 0.0f, 1.0f);
        int r = (int)(col0_r + col_delta_r * t);
        int g = (int)(col0_g + col_delta_g * t);
        int b = (int)(col0_b + col_delta_b * t);
        vert->col = (r << IM_COL32_R_SHIFT) | (g << IM_COL32_G_SHIFT) | (b << IM_COL32_B_SHIFT) | (vert->col & IM_COL32_A_MASK);
    }
}

// Distribute UV over (a, b) rectangle
void ImGui::ShadeVertsLinearUV(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, const ImVec2& a, const ImVec2& b, const ImVec2& uv_a, const ImVec2& uv_b, bool clamp)
{
    const ImVec2 size = b - a;
    const ImVec2 uv_size = uv_b - uv_a;
    const ImVec2 scale = ImVec2(
        size.x != 0.0f ? (uv_size.x / size.x) : 0.0f,
        size.y != 0.0f ? (uv_size.y / size.y) : 0.0f);

    ImDrawVert* vert_start = draw_list->VtxBuffer.Data + vert_start_idx;
    ImDrawVert* vert_end = draw_list->VtxBuffer.Data + vert_end_idx;
    if (clamp)
    {
        const ImVec2 min = ImMin(uv_a, uv_b);
        const ImVec2 max = ImMax(uv_a, uv_b);
        for (ImDrawVert* vertex = vert_start; vertex < vert_end; ++vertex)
            vertex->uv = ImClamp(uv_a + ImMul(ImVec2(vertex->pos.x, vertex->pos.y) - a, scale), min, max);
    }
    else
    {
        for (ImDrawVert* vertex = vert_start; vertex < vert_end; ++vertex)
            vertex->uv = uv_a + ImMul(ImVec2(vertex->pos.x, vertex->pos.y) - a, scale);
    }
}

//-----------------------------------------------------------------------------
// [SECTION] ImFontConfig
//-----------------------------------------------------------------------------

ImFontConfig::ImFontConfig()
{
    FontData = NULL;
    FontDataSize = 0;
    FontDataOwnedByAtlas = true;
    FontNo = 0;
    SizePixels = 0.0f;
    OversampleH = 3; // FIXME: 2 may be a better default?
    OversampleV = 1;
    PixelSnapH = false;
    GlyphExtraSpacing = ImVec2(0.0f, 0.0f);
    GlyphOffset = ImVec2(0.0f, 0.0f);
    GlyphRanges = NULL;
    GlyphMinAdvanceX = 0.0f;
    GlyphMaxAdvanceX = FLT_MAX;
    MergeMode = false;
    RasterizerFlags = 0x00;
    RasterizerMultiply = 1.0f;
    EllipsisChar = (ImWchar)-1;
    memset(Name, 0, sizeof(Name));
    DstFont = NULL;
}

//-----------------------------------------------------------------------------
// [SECTION] ImFontAtlas
//-----------------------------------------------------------------------------

// A work of art lies ahead! (. = white layer, X = black layer, others are blank)
// The 2x2 white texels on the top left are the ones we'll use everywhere in Dear ImGui to render filled shapes.
const int FONT_ATLAS_DEFAULT_TEX_DATA_W = 108; // Actual texture will be 2 times that + 1 spacing.
const int FONT_ATLAS_DEFAULT_TEX_DATA_H = 27;
static const char FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS[FONT_ATLAS_DEFAULT_TEX_DATA_W * FONT_ATLAS_DEFAULT_TEX_DATA_H + 1] =
{
    "..-         -XXXXXXX-    X    -           X           -XXXXXXX          -          XXXXXXX-     XX          "
    "..-         -X.....X-   X.X   -          X.X          -X.....X          -          X.....X-    X..X         "
    "---         -XXX.XXX-  X...X  -         X...X         -X....X           -           X....X-    X..X         "
    "X           -  X.X  - X.....X -        X.....X        -X...X            -            X...X-    X..X         "
    "XX          -  X.X  -X.......X-       X.......X       -X..X.X           -           X.X..X-    X..X         "
    "X.X         -  X.X  -XXXX.XXXX-       XXXX.XXXX       -X.X X.X          -          X.X X.X-    X..XXX       "
    "X..X        -  X.X  -   X.X   -          X.X          -XX   X.X         -         X.X   XX-    X..X..XXX    "
    "X...X       -  X.X  -   X.X   -    XX    X.X    XX    -      X.X        -        X.X      -    X..X..X..XX  "
    "X....X      -  X.X  -   X.X   -   X.X    X.X    X.X   -       X.X       -       X.X       -    X..X..X..X.X "
    "X.....X     -  X.X  -   X.X   -  X..X    X.X    X..X  -        X.X      -      X.X        -XXX X..X..X..X..X"
    "X......X    -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -         X.X   XX-XX   X.X         -X..XX........X..X"
    "X.......X   -  X.X  -   X.X   -X.....................X-          X.X X.X-X.X X.X          -X...X...........X"
    "X........X  -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -           X.X..X-X..X.X           - X..............X"
    "X.........X -XXX.XXX-   X.X   -  X..X    X.X    X..X  -            X...X-X...X            -  X.............X"
    "X..........X-X.....X-   X.X   -   X.X    X.X    X.X   -           X....X-X....X           -  X.............X"
    "X......XXXXX-XXXXXXX-   X.X   -    XX    X.X    XX    -          X.....X-X.....X          -   X............X"
    "X...X..X    ---------   X.X   -          X.X          -          XXXXXXX-XXXXXXX          -   X...........X "
    "X..X X..X   -       -XXXX.XXXX-       XXXX.XXXX       -------------------------------------    X..........X "
    "X.X  X..X   -       -X.......X-       X.......X       -    XX           XX    -           -    X..........X "
    "XX    X..X  -       - X.....X -        X.....X        -   X.X           X.X   -           -     X........X  "
    "      X..X          -  X...X  -         X...X         -  X..X           X..X  -           -     X........X  "
    "       XX           -   X.X   -          X.X          - X...XXXXXXXXXXXXX...X -           -     XXXXXXXXXX  "
    "------------        -    X    -           X           -X.....................X-           ------------------"
    "                    ----------------------------------- X...XXXXXXXXXXXXX...X -                             "
    "                                                      -  X..X           X..X  -                             "
    "                                                      -   X.X           X.X   -                             "
    "                                                      -    XX           XX    -                             "
};

static const ImVec2 FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[ImGuiMouseCursor_COUNT][3] =
{
    // Pos ........ Size ......... Offset ......
    { ImVec2( 0,3), ImVec2(12,19), ImVec2( 0, 0) }, // ImGuiMouseCursor_Arrow
    { ImVec2(13,0), ImVec2( 7,16), ImVec2( 1, 8) }, // ImGuiMouseCursor_TextInput
    { ImVec2(31,0), ImVec2(23,23), ImVec2(11,11) }, // ImGuiMouseCursor_ResizeAll
    { ImVec2(21,0), ImVec2( 9,23), ImVec2( 4,11) }, // ImGuiMouseCursor_ResizeNS
    { ImVec2(55,18),ImVec2(23, 9), ImVec2(11, 4) }, // ImGuiMouseCursor_ResizeEW
    { ImVec2(73,0), ImVec2(17,17), ImVec2( 8, 8) }, // ImGuiMouseCursor_ResizeNESW
    { ImVec2(55,0), ImVec2(17,17), ImVec2( 8, 8) }, // ImGuiMouseCursor_ResizeNWSE
    { ImVec2(91,0), ImVec2(17,22), ImVec2( 5, 0) }, // ImGuiMouseCursor_Hand
};

ImFontAtlas::ImFontAtlas()
{
    Locked = false;
    Flags = ImFontAtlasFlags_None;
    TexID = (ImTextureID)NULL;
    TexDesiredWidth = 0;
    TexGlyphPadding = 1;

    TexPixelsAlpha8 = NULL;
    TexPixelsRGBA32 = NULL;
    TexWidth = TexHeight = 0;
    TexUvScale = ImVec2(0.0f, 0.0f);
    TexUvWhitePixel = ImVec2(0.0f, 0.0f);
    PackIdMouseCursors = PackIdLines = -1;
}

ImFontAtlas::~ImFontAtlas()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    Clear();
}

void    ImFontAtlas::ClearInputData()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    for (int i = 0; i < ConfigData.Size; i++)
        if (ConfigData[i].FontData && ConfigData[i].FontDataOwnedByAtlas)
        {
            IM_FREE(ConfigData[i].FontData);
            ConfigData[i].FontData = NULL;
        }

    // When clearing this we lose access to the font name and other information used to build the font.
    for (int i = 0; i < Fonts.Size; i++)
        if (Fonts[i]->ConfigData >= ConfigData.Data && Fonts[i]->ConfigData < ConfigData.Data + ConfigData.Size)
        {
            Fonts[i]->ConfigData = NULL;
            Fonts[i]->ConfigDataCount = 0;
        }
    ConfigData.clear();
    CustomRects.clear();
    PackIdMouseCursors = PackIdLines = -1;
}

void    ImFontAtlas::ClearTexData()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    if (TexPixelsAlpha8)
        IM_FREE(TexPixelsAlpha8);
    if (TexPixelsRGBA32)
        IM_FREE(TexPixelsRGBA32);
    TexPixelsAlpha8 = NULL;
    TexPixelsRGBA32 = NULL;
}

void    ImFontAtlas::ClearFonts()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    for (int i = 0; i < Fonts.Size; i++)
        IM_DELETE(Fonts[i]);
    Fonts.clear();
}

void    ImFontAtlas::Clear()
{
    ClearInputData();
    ClearTexData();
    ClearFonts();
}

void    ImFontAtlas::GetTexDataAsAlpha8(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
    // Build atlas on demand
    if (TexPixelsAlpha8 == NULL)
    {
        if (ConfigData.empty())
            AddFontDefault();
        Build();
    }

    *out_pixels = TexPixelsAlpha8;
    if (out_width) *out_width = TexWidth;
    if (out_height) *out_height = TexHeight;
    if (out_bytes_per_pixel) *out_bytes_per_pixel = 1;
}

void    ImFontAtlas::GetTexDataAsRGBA32(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel)
{
    // Convert to RGBA32 format on demand
    // Although it is likely to be the most commonly used format, our font rendering is 1 channel / 8 bpp
    if (!TexPixelsRGBA32)
    {
        unsigned char* pixels = NULL;
        GetTexDataAsAlpha8(&pixels, NULL, NULL);
        if (pixels)
        {
            TexPixelsRGBA32 = (unsigned int*)IM_ALLOC((size_t)TexWidth * (size_t)TexHeight * 4);
            const unsigned char* src = pixels;
            unsigned int* dst = TexPixelsRGBA32;
            for (int n = TexWidth * TexHeight; n > 0; n--)
                *dst++ = IM_COL32(255, 255, 255, (unsigned int)(*src++));
        }
    }

    *out_pixels = (unsigned char*)TexPixelsRGBA32;
    if (out_width) *out_width = TexWidth;
    if (out_height) *out_height = TexHeight;
    if (out_bytes_per_pixel) *out_bytes_per_pixel = 4;
}

ImFont* ImFontAtlas::AddFont(const ImFontConfig* font_cfg)
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    IM_ASSERT(font_cfg->FontData != NULL && font_cfg->FontDataSize > 0);
    IM_ASSERT(font_cfg->SizePixels > 0.0f);

    // Create new font
    if (!font_cfg->MergeMode)
        Fonts.push_back(IM_NEW(ImFont));
    else
        IM_ASSERT(!Fonts.empty() && "Cannot use MergeMode for the first font"); // When using MergeMode make sure that a font has already been added before. You can use ImGui::GetIO().Fonts->AddFontDefault() to add the default imgui font.

    ConfigData.push_back(*font_cfg);
    ImFontConfig& new_font_cfg = ConfigData.back();
    if (new_font_cfg.DstFont == NULL)
        new_font_cfg.DstFont = Fonts.back();
    if (!new_font_cfg.FontDataOwnedByAtlas)
    {
        new_font_cfg.FontData = IM_ALLOC(new_font_cfg.FontDataSize);
        new_font_cfg.FontDataOwnedByAtlas = true;
        memcpy(new_font_cfg.FontData, font_cfg->FontData, (size_t)new_font_cfg.FontDataSize);
    }

    if (new_font_cfg.DstFont->EllipsisChar == (ImWchar)-1)
        new_font_cfg.DstFont->EllipsisChar = font_cfg->EllipsisChar;

    // Invalidate texture
    ClearTexData();
    return new_font_cfg.DstFont;
}

// Default font TTF is compressed with stb_compress then base85 encoded (see misc/fonts/binary_to_compressed_c.cpp for encoder)
static unsigned int stb_decompress_length(const unsigned char* input);
static unsigned int stb_decompress(unsigned char* output, const unsigned char* input, unsigned int length);
static const char*  GetDefaultCompressedFontDataTTFBase85();
static unsigned int Decode85Byte(char c)                                    { return c >= '\\' ? c-36 : c-35; }
static void         Decode85(const unsigned char* src, unsigned char* dst)
{
    while (*src)
    {
        unsigned int tmp = Decode85Byte(src[0]) + 85 * (Decode85Byte(src[1]) + 85 * (Decode85Byte(src[2]) + 85 * (Decode85Byte(src[3]) + 85 * Decode85Byte(src[4]))));
        dst[0] = ((tmp >> 0) & 0xFF); dst[1] = ((tmp >> 8) & 0xFF); dst[2] = ((tmp >> 16) & 0xFF); dst[3] = ((tmp >> 24) & 0xFF);   // We can't assume little-endianness.
        src += 5;
        dst += 4;
    }
}

// Load embedded ProggyClean.ttf at size 13, disable oversampling
ImFont* ImFontAtlas::AddFontDefault(const ImFontConfig* font_cfg_template)
{
    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    if (!font_cfg_template)
    {
        font_cfg.OversampleH = font_cfg.OversampleV = 1;
        font_cfg.PixelSnapH = true;
    }
    if (font_cfg.SizePixels <= 0.0f)
        font_cfg.SizePixels = 13.0f * 1.0f;
    if (font_cfg.Name[0] == '\0')
        ImFormatString(font_cfg.Name, IM_ARRAYSIZE(font_cfg.Name), "ProggyClean.ttf, %dpx", (int)font_cfg.SizePixels);
    font_cfg.EllipsisChar = (ImWchar)0x0085;

    const char* ttf_compressed_base85 = GetDefaultCompressedFontDataTTFBase85();
    const ImWchar* glyph_ranges = font_cfg.GlyphRanges != NULL ? font_cfg.GlyphRanges : GetGlyphRangesDefault();
    ImFont* font = AddFontFromMemoryCompressedBase85TTF(ttf_compressed_base85, font_cfg.SizePixels, &font_cfg, glyph_ranges);
    font->DisplayOffset.y = 1.0f;
    return font;
}

ImFont* ImFontAtlas::AddFontFromFileTTF(const char* filename, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    size_t data_size = 0;
    void* data = ImFileLoadToMemory(filename, "rb", &data_size, 0);
    if (!data)
    {
        IM_ASSERT_USER_ERROR(0, "Could not load font file!");
        return NULL;
    }
    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    if (font_cfg.Name[0] == '\0')
    {
        // Store a short copy of filename into into the font name for convenience
        const char* p;
        for (p = filename + strlen(filename); p > filename && p[-1] != '/' && p[-1] != '\\'; p--) {}
        ImFormatString(font_cfg.Name, IM_ARRAYSIZE(font_cfg.Name), "%s, %.0fpx", p, size_pixels);
    }
    return AddFontFromMemoryTTF(data, (int)data_size, size_pixels, &font_cfg, glyph_ranges);
}

// NB: Transfer ownership of 'ttf_data' to ImFontAtlas, unless font_cfg_template->FontDataOwnedByAtlas == false. Owned TTF buffer will be deleted after Build().
ImFont* ImFontAtlas::AddFontFromMemoryTTF(void* ttf_data, int ttf_size, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    IM_ASSERT(font_cfg.FontData == NULL);
    font_cfg.FontData = ttf_data;
    font_cfg.FontDataSize = ttf_size;
    font_cfg.SizePixels = size_pixels;
    if (glyph_ranges)
        font_cfg.GlyphRanges = glyph_ranges;
    return AddFont(&font_cfg);
}

ImFont* ImFontAtlas::AddFontFromMemoryCompressedTTF(const void* compressed_ttf_data, int compressed_ttf_size, float size_pixels, const ImFontConfig* font_cfg_template, const ImWchar* glyph_ranges)
{
    const unsigned int buf_decompressed_size = stb_decompress_length((const unsigned char*)compressed_ttf_data);
    unsigned char* buf_decompressed_data = (unsigned char*)IM_ALLOC(buf_decompressed_size);
    stb_decompress(buf_decompressed_data, (const unsigned char*)compressed_ttf_data, (unsigned int)compressed_ttf_size);

    ImFontConfig font_cfg = font_cfg_template ? *font_cfg_template : ImFontConfig();
    IM_ASSERT(font_cfg.FontData == NULL);
    font_cfg.FontDataOwnedByAtlas = true;
    return AddFontFromMemoryTTF(buf_decompressed_data, (int)buf_decompressed_size, size_pixels, &font_cfg, glyph_ranges);
}

ImFont* ImFontAtlas::AddFontFromMemoryCompressedBase85TTF(const char* compressed_ttf_data_base85, float size_pixels, const ImFontConfig* font_cfg, const ImWchar* glyph_ranges)
{
    int compressed_ttf_size = (((int)strlen(compressed_ttf_data_base85) + 4) / 5) * 4;
    void* compressed_ttf = IM_ALLOC((size_t)compressed_ttf_size);
    Decode85((const unsigned char*)compressed_ttf_data_base85, (unsigned char*)compressed_ttf);
    ImFont* font = AddFontFromMemoryCompressedTTF(compressed_ttf, compressed_ttf_size, size_pixels, font_cfg, glyph_ranges);
    IM_FREE(compressed_ttf);
    return font;
}

int ImFontAtlas::AddCustomRectRegular(int width, int height)
{
    IM_ASSERT(width > 0 && width <= 0xFFFF);
    IM_ASSERT(height > 0 && height <= 0xFFFF);
    ImFontAtlasCustomRect r;
    r.Width = (unsigned short)width;
    r.Height = (unsigned short)height;
    CustomRects.push_back(r);
    return CustomRects.Size - 1; // Return index
}

int ImFontAtlas::AddCustomRectFontGlyph(ImFont* font, ImWchar id, int width, int height, float advance_x, const ImVec2& offset)
{
#ifdef IMGUI_USE_WCHAR32
    IM_ASSERT(id <= IM_UNICODE_CODEPOINT_MAX);
#endif
    IM_ASSERT(font != NULL);
    IM_ASSERT(width > 0 && width <= 0xFFFF);
    IM_ASSERT(height > 0 && height <= 0xFFFF);
    ImFontAtlasCustomRect r;
    r.Width = (unsigned short)width;
    r.Height = (unsigned short)height;
    r.GlyphID = id;
    r.GlyphAdvanceX = advance_x;
    r.GlyphOffset = offset;
    r.Font = font;
    CustomRects.push_back(r);
    return CustomRects.Size - 1; // Return index
}

void ImFontAtlas::CalcCustomRectUV(const ImFontAtlasCustomRect* rect, ImVec2* out_uv_min, ImVec2* out_uv_max) const
{
    IM_ASSERT(TexWidth > 0 && TexHeight > 0);   // Font atlas needs to be built before we can calculate UV coordinates
    IM_ASSERT(rect->IsPacked());                // Make sure the rectangle has been packed
    *out_uv_min = ImVec2((float)rect->X * TexUvScale.x, (float)rect->Y * TexUvScale.y);
    *out_uv_max = ImVec2((float)(rect->X + rect->Width) * TexUvScale.x, (float)(rect->Y + rect->Height) * TexUvScale.y);
}

bool ImFontAtlas::GetMouseCursorTexData(ImGuiMouseCursor cursor_type, ImVec2* out_offset, ImVec2* out_size, ImVec2 out_uv_border[2], ImVec2 out_uv_fill[2])
{
    if (cursor_type <= ImGuiMouseCursor_None || cursor_type >= ImGuiMouseCursor_COUNT)
        return false;
    if (Flags & ImFontAtlasFlags_NoMouseCursors)
        return false;

    IM_ASSERT(PackIdMouseCursors != -1);
    ImFontAtlasCustomRect* r = GetCustomRectByIndex(PackIdMouseCursors);
    ImVec2 pos = FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[cursor_type][0] + ImVec2((float)r->X, (float)r->Y);
    ImVec2 size = FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[cursor_type][1];
    *out_size = size;
    *out_offset = FONT_ATLAS_DEFAULT_TEX_CURSOR_DATA[cursor_type][2];
    out_uv_border[0] = (pos) * TexUvScale;
    out_uv_border[1] = (pos + size) * TexUvScale;
    pos.x += FONT_ATLAS_DEFAULT_TEX_DATA_W + 1;
    out_uv_fill[0] = (pos) * TexUvScale;
    out_uv_fill[1] = (pos + size) * TexUvScale;
    return true;
}

bool    ImFontAtlas::Build()
{
    IM_ASSERT(!Locked && "Cannot modify a locked ImFontAtlas between NewFrame() and EndFrame/Render()!");
    return ImFontAtlasBuildWithStbTruetype(this);
}

void    ImFontAtlasBuildMultiplyCalcLookupTable(unsigned char out_table[256], float in_brighten_factor)
{
    for (unsigned int i = 0; i < 256; i++)
    {
        unsigned int value = (unsigned int)(i * in_brighten_factor);
        out_table[i] = value > 255 ? 255 : (value & 0xFF);
    }
}

void    ImFontAtlasBuildMultiplyRectAlpha8(const unsigned char table[256], unsigned char* pixels, int x, int y, int w, int h, int stride)
{
    unsigned char* data = pixels + x + y * stride;
    for (int j = h; j > 0; j--, data += stride)
        for (int i = 0; i < w; i++)
            data[i] = table[data[i]];
}

// Temporary data for one source font (multiple source fonts can be merged into one destination ImFont)
// (C++03 doesn't allow instancing ImVector<> with function-local types so we declare the type here.)
struct ImFontBuildSrcData
{
    stbtt_fontinfo      FontInfo;
    stbtt_pack_range    PackRange;          // Hold the list of codepoints to pack (essentially points to Codepoints.Data)
    stbrp_rect*         Rects;              // Rectangle to pack. We first fill in their size and the packer will give us their position.
    stbtt_packedchar*   PackedChars;        // Output glyphs
    const ImWchar*      SrcRanges;          // Ranges as requested by user (user is allowed to request too much, e.g. 0x0020..0xFFFF)
    int                 DstIndex;           // Index into atlas->Fonts[] and dst_tmp_array[]
    int                 GlyphsHighest;      // Highest requested codepoint
    int                 GlyphsCount;        // Glyph count (excluding missing glyphs and glyphs already set by an earlier source font)
    ImBitVector         GlyphsSet;          // Glyph bit map (random access, 1-bit per codepoint. This will be a maximum of 8KB)
    ImVector<int>       GlyphsList;         // Glyph codepoints list (flattened version of GlyphsMap)
};

// Temporary data for one destination ImFont* (multiple source fonts can be merged into one destination ImFont)
struct ImFontBuildDstData
{
    int                 SrcCount;           // Number of source fonts targeting this destination font.
    int                 GlyphsHighest;
    int                 GlyphsCount;
    ImBitVector         GlyphsSet;          // This is used to resolve collision when multiple sources are merged into a same destination font.
};

static void UnpackBitVectorToFlatIndexList(const ImBitVector* in, ImVector<int>* out)
{
    IM_ASSERT(sizeof(in->Storage.Data[0]) == sizeof(int));
    const ImU32* it_begin = in->Storage.begin();
    const ImU32* it_end = in->Storage.end();
    for (const ImU32* it = it_begin; it < it_end; it++)
        if (ImU32 entries_32 = *it)
            for (ImU32 bit_n = 0; bit_n < 32; bit_n++)
                if (entries_32 & ((ImU32)1 << bit_n))
                    out->push_back((int)(((it - it_begin) << 5) + bit_n));
}

bool    ImFontAtlasBuildWithStbTruetype(ImFontAtlas* atlas)
{
    IM_ASSERT(atlas->ConfigData.Size > 0);

    ImFontAtlasBuildInit(atlas);

    // Clear atlas
    atlas->TexID = (ImTextureID)NULL;
    atlas->TexWidth = atlas->TexHeight = 0;
    atlas->TexUvScale = ImVec2(0.0f, 0.0f);
    atlas->TexUvWhitePixel = ImVec2(0.0f, 0.0f);
    atlas->ClearTexData();

    // Temporary storage for building
    ImVector<ImFontBuildSrcData> src_tmp_array;
    ImVector<ImFontBuildDstData> dst_tmp_array;
    src_tmp_array.resize(atlas->ConfigData.Size);
    dst_tmp_array.resize(atlas->Fonts.Size);
    memset(src_tmp_array.Data, 0, (size_t)src_tmp_array.size_in_bytes());
    memset(dst_tmp_array.Data, 0, (size_t)dst_tmp_array.size_in_bytes());

    // 1. Initialize font loading structure, check font data validity
    for (int src_i = 0; src_i < atlas->ConfigData.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        ImFontConfig& cfg = atlas->ConfigData[src_i];
        IM_ASSERT(cfg.DstFont && (!cfg.DstFont->IsLoaded() || cfg.DstFont->ContainerAtlas == atlas));

        // Find index from cfg.DstFont (we allow the user to set cfg.DstFont. Also it makes casual debugging nicer than when storing indices)
        src_tmp.DstIndex = -1;
        for (int output_i = 0; output_i < atlas->Fonts.Size && src_tmp.DstIndex == -1; output_i++)
            if (cfg.DstFont == atlas->Fonts[output_i])
                src_tmp.DstIndex = output_i;
        IM_ASSERT(src_tmp.DstIndex != -1); // cfg.DstFont not pointing within atlas->Fonts[] array?
        if (src_tmp.DstIndex == -1)
            return false;

        // Initialize helper structure for font loading and verify that the TTF/OTF data is correct
        const int font_offset = stbtt_GetFontOffsetForIndex((unsigned char*)cfg.FontData, cfg.FontNo);
        IM_ASSERT(font_offset >= 0 && "FontData is incorrect, or FontNo cannot be found.");
        if (!stbtt_InitFont(&src_tmp.FontInfo, (unsigned char*)cfg.FontData, font_offset))
            return false;

        // Measure highest codepoints
        ImFontBuildDstData& dst_tmp = dst_tmp_array[src_tmp.DstIndex];
        src_tmp.SrcRanges = cfg.GlyphRanges ? cfg.GlyphRanges : atlas->GetGlyphRangesDefault();
        for (const ImWchar* src_range = src_tmp.SrcRanges; src_range[0] && src_range[1]; src_range += 2)
            src_tmp.GlyphsHighest = ImMax(src_tmp.GlyphsHighest, (int)src_range[1]);
        dst_tmp.SrcCount++;
        dst_tmp.GlyphsHighest = ImMax(dst_tmp.GlyphsHighest, src_tmp.GlyphsHighest);
    }

    // 2. For every requested codepoint, check for their presence in the font data, and handle redundancy or overlaps between source fonts to avoid unused glyphs.
    int total_glyphs_count = 0;
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        ImFontBuildDstData& dst_tmp = dst_tmp_array[src_tmp.DstIndex];
        src_tmp.GlyphsSet.Create(src_tmp.GlyphsHighest + 1);
        if (dst_tmp.GlyphsSet.Storage.empty())
            dst_tmp.GlyphsSet.Create(dst_tmp.GlyphsHighest + 1);

        for (const ImWchar* src_range = src_tmp.SrcRanges; src_range[0] && src_range[1]; src_range += 2)
            for (unsigned int codepoint = src_range[0]; codepoint <= src_range[1]; codepoint++)
            {
                if (dst_tmp.GlyphsSet.TestBit(codepoint))    // Don't overwrite existing glyphs. We could make this an option for MergeMode (e.g. MergeOverwrite==true)
                    continue;
                if (!stbtt_FindGlyphIndex(&src_tmp.FontInfo, codepoint))    // It is actually in the font?
                    continue;

                // Add to avail set/counters
                src_tmp.GlyphsCount++;
                dst_tmp.GlyphsCount++;
                src_tmp.GlyphsSet.SetBit(codepoint);
                dst_tmp.GlyphsSet.SetBit(codepoint);
                total_glyphs_count++;
            }
    }

    // 3. Unpack our bit map into a flat list (we now have all the Unicode points that we know are requested _and_ available _and_ not overlapping another)
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        src_tmp.GlyphsList.reserve(src_tmp.GlyphsCount);
        UnpackBitVectorToFlatIndexList(&src_tmp.GlyphsSet, &src_tmp.GlyphsList);
        src_tmp.GlyphsSet.Clear();
        IM_ASSERT(src_tmp.GlyphsList.Size == src_tmp.GlyphsCount);
    }
    for (int dst_i = 0; dst_i < dst_tmp_array.Size; dst_i++)
        dst_tmp_array[dst_i].GlyphsSet.Clear();
    dst_tmp_array.clear();

    // Allocate packing character data and flag packed characters buffer as non-packed (x0=y0=x1=y1=0)
    // (We technically don't need to zero-clear buf_rects, but let's do it for the sake of sanity)
    ImVector<stbrp_rect> buf_rects;
    ImVector<stbtt_packedchar> buf_packedchars;
    buf_rects.resize(total_glyphs_count);
    buf_packedchars.resize(total_glyphs_count);
    memset(buf_rects.Data, 0, (size_t)buf_rects.size_in_bytes());
    memset(buf_packedchars.Data, 0, (size_t)buf_packedchars.size_in_bytes());

    // 4. Gather glyphs sizes so we can pack them in our virtual canvas.
    int total_surface = 0;
    int buf_rects_out_n = 0;
    int buf_packedchars_out_n = 0;
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        if (src_tmp.GlyphsCount == 0)
            continue;

        src_tmp.Rects = &buf_rects[buf_rects_out_n];
        src_tmp.PackedChars = &buf_packedchars[buf_packedchars_out_n];
        buf_rects_out_n += src_tmp.GlyphsCount;
        buf_packedchars_out_n += src_tmp.GlyphsCount;

        // Convert our ranges in the format stb_truetype wants
        ImFontConfig& cfg = atlas->ConfigData[src_i];
        src_tmp.PackRange.font_size = cfg.SizePixels;
        src_tmp.PackRange.first_unicode_codepoint_in_range = 0;
        src_tmp.PackRange.array_of_unicode_codepoints = src_tmp.GlyphsList.Data;
        src_tmp.PackRange.num_chars = src_tmp.GlyphsList.Size;
        src_tmp.PackRange.chardata_for_range = src_tmp.PackedChars;
        src_tmp.PackRange.h_oversample = (unsigned char)cfg.OversampleH;
        src_tmp.PackRange.v_oversample = (unsigned char)cfg.OversampleV;

        // Gather the sizes of all rectangles we will need to pack (this loop is based on stbtt_PackFontRangesGatherRects)
        const float scale = (cfg.SizePixels > 0) ? stbtt_ScaleForPixelHeight(&src_tmp.FontInfo, cfg.SizePixels) : stbtt_ScaleForMappingEmToPixels(&src_tmp.FontInfo, -cfg.SizePixels);
        const int padding = atlas->TexGlyphPadding;
        for (int glyph_i = 0; glyph_i < src_tmp.GlyphsList.Size; glyph_i++)
        {
            int x0, y0, x1, y1;
            const int glyph_index_in_font = stbtt_FindGlyphIndex(&src_tmp.FontInfo, src_tmp.GlyphsList[glyph_i]);
            IM_ASSERT(glyph_index_in_font != 0);
            stbtt_GetGlyphBitmapBoxSubpixel(&src_tmp.FontInfo, glyph_index_in_font, scale * cfg.OversampleH, scale * cfg.OversampleV, 0, 0, &x0, &y0, &x1, &y1);
            src_tmp.Rects[glyph_i].w = (stbrp_coord)(x1 - x0 + padding + cfg.OversampleH - 1);
            src_tmp.Rects[glyph_i].h = (stbrp_coord)(y1 - y0 + padding + cfg.OversampleV - 1);
            total_surface += src_tmp.Rects[glyph_i].w * src_tmp.Rects[glyph_i].h;
        }
    }

    // We need a width for the skyline algorithm, any width!
    // The exact width doesn't really matter much, but some API/GPU have texture size limitations and increasing width can decrease height.
    // User can override TexDesiredWidth and TexGlyphPadding if they wish, otherwise we use a simple heuristic to select the width based on expected surface.
    const int surface_sqrt = (int)ImSqrt((float)total_surface) + 1;
    atlas->TexHeight = 0;
    if (atlas->TexDesiredWidth > 0)
        atlas->TexWidth = atlas->TexDesiredWidth;
    else
        atlas->TexWidth = (surface_sqrt >= 4096 * 0.7f) ? 4096 : (surface_sqrt >= 2048 * 0.7f) ? 2048 : (surface_sqrt >= 1024 * 0.7f) ? 1024 : 512;

    // 5. Start packing
    // Pack our extra data rectangles first, so it will be on the upper-left corner of our texture (UV will have small values).
    const int TEX_HEIGHT_MAX = 1024 * 32;
    stbtt_pack_context spc = {};
    stbtt_PackBegin(&spc, NULL, atlas->TexWidth, TEX_HEIGHT_MAX, 0, atlas->TexGlyphPadding, NULL);
    ImFontAtlasBuildPackCustomRects(atlas, spc.pack_info);

    // 6. Pack each source font. No rendering yet, we are working with rectangles in an infinitely tall texture at this point.
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        if (src_tmp.GlyphsCount == 0)
            continue;

        stbrp_pack_rects((stbrp_context*)spc.pack_info, src_tmp.Rects, src_tmp.GlyphsCount);

        // Extend texture height and mark missing glyphs as non-packed so we won't render them.
        // FIXME: We are not handling packing failure here (would happen if we got off TEX_HEIGHT_MAX or if a single if larger than TexWidth?)
        for (int glyph_i = 0; glyph_i < src_tmp.GlyphsCount; glyph_i++)
            if (src_tmp.Rects[glyph_i].was_packed)
                atlas->TexHeight = ImMax(atlas->TexHeight, src_tmp.Rects[glyph_i].y + src_tmp.Rects[glyph_i].h);
    }

    // 7. Allocate texture
    atlas->TexHeight = (atlas->Flags & ImFontAtlasFlags_NoPowerOfTwoHeight) ? (atlas->TexHeight + 1) : ImUpperPowerOfTwo(atlas->TexHeight);
    atlas->TexUvScale = ImVec2(1.0f / atlas->TexWidth, 1.0f / atlas->TexHeight);
    atlas->TexPixelsAlpha8 = (unsigned char*)IM_ALLOC(atlas->TexWidth * atlas->TexHeight);
    memset(atlas->TexPixelsAlpha8, 0, atlas->TexWidth * atlas->TexHeight);
    spc.pixels = atlas->TexPixelsAlpha8;
    spc.height = atlas->TexHeight;

    // 8. Render/rasterize font characters into the texture
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontConfig& cfg = atlas->ConfigData[src_i];
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        if (src_tmp.GlyphsCount == 0)
            continue;

        stbtt_PackFontRangesRenderIntoRects(&spc, &src_tmp.FontInfo, &src_tmp.PackRange, 1, src_tmp.Rects);

        // Apply multiply operator
        if (cfg.RasterizerMultiply != 1.0f)
        {
            unsigned char multiply_table[256];
            ImFontAtlasBuildMultiplyCalcLookupTable(multiply_table, cfg.RasterizerMultiply);
            stbrp_rect* r = &src_tmp.Rects[0];
            for (int glyph_i = 0; glyph_i < src_tmp.GlyphsCount; glyph_i++, r++)
                if (r->was_packed)
                    ImFontAtlasBuildMultiplyRectAlpha8(multiply_table, atlas->TexPixelsAlpha8, r->x, r->y, r->w, r->h, atlas->TexWidth * 1);
        }
        src_tmp.Rects = NULL;
    }

    // End packing
    stbtt_PackEnd(&spc);
    buf_rects.clear();

    // 9. Setup ImFont and glyphs for runtime
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
    {
        ImFontBuildSrcData& src_tmp = src_tmp_array[src_i];
        if (src_tmp.GlyphsCount == 0)
            continue;

        // When merging fonts with MergeMode=true:
        // - We can have multiple input fonts writing into a same destination font.
        // - dst_font->ConfigData is != from cfg which is our source configuration.
        ImFontConfig& cfg = atlas->ConfigData[src_i];
        ImFont* dst_font = cfg.DstFont;

        const float font_scale = stbtt_ScaleForPixelHeight(&src_tmp.FontInfo, cfg.SizePixels);
        int unscaled_ascent, unscaled_descent, unscaled_line_gap;
        stbtt_GetFontVMetrics(&src_tmp.FontInfo, &unscaled_ascent, &unscaled_descent, &unscaled_line_gap);

        const float ascent = ImFloor(unscaled_ascent * font_scale + ((unscaled_ascent > 0.0f) ? +1 : -1));
        const float descent = ImFloor(unscaled_descent * font_scale + ((unscaled_descent > 0.0f) ? +1 : -1));
        ImFontAtlasBuildSetupFont(atlas, dst_font, &cfg, ascent, descent);
        const float font_off_x = cfg.GlyphOffset.x;
        const float font_off_y = cfg.GlyphOffset.y + IM_ROUND(dst_font->Ascent);

        for (int glyph_i = 0; glyph_i < src_tmp.GlyphsCount; glyph_i++)
        {
            // Register glyph
            const int codepoint = src_tmp.GlyphsList[glyph_i];
            const stbtt_packedchar& pc = src_tmp.PackedChars[glyph_i];
            stbtt_aligned_quad q;
            float unused_x = 0.0f, unused_y = 0.0f;
            stbtt_GetPackedQuad(src_tmp.PackedChars, atlas->TexWidth, atlas->TexHeight, glyph_i, &unused_x, &unused_y, &q, 0);
            dst_font->AddGlyph(&cfg, (ImWchar)codepoint, q.x0 + font_off_x, q.y0 + font_off_y, q.x1 + font_off_x, q.y1 + font_off_y, q.s0, q.t0, q.s1, q.t1, pc.xadvance);
        }
    }

    // Cleanup temporary (ImVector doesn't honor destructor)
    for (int src_i = 0; src_i < src_tmp_array.Size; src_i++)
        src_tmp_array[src_i].~ImFontBuildSrcData();

    ImFontAtlasBuildFinish(atlas);
    return true;
}

void ImFontAtlasBuildSetupFont(ImFontAtlas* atlas, ImFont* font, ImFontConfig* font_config, float ascent, float descent)
{
    if (!font_config->MergeMode)
    {
        font->ClearOutputData();
        font->FontSize = font_config->SizePixels;
        font->ConfigData = font_config;
        font->ConfigDataCount = 0;
        font->ContainerAtlas = atlas;
        font->Ascent = ascent;
        font->Descent = descent;
    }
    font->ConfigDataCount++;
}

void ImFontAtlasBuildPackCustomRects(ImFontAtlas* atlas, void* stbrp_context_opaque)
{
    stbrp_context* pack_context = (stbrp_context*)stbrp_context_opaque;
    IM_ASSERT(pack_context != NULL);

    ImVector<ImFontAtlasCustomRect>& user_rects = atlas->CustomRects;
    IM_ASSERT(user_rects.Size >= 1); // We expect at least the default custom rects to be registered, else something went wrong.

    ImVector<stbrp_rect> pack_rects;
    pack_rects.resize(user_rects.Size);
    memset(pack_rects.Data, 0, (size_t)pack_rects.size_in_bytes());
    for (int i = 0; i < user_rects.Size; i++)
    {
        pack_rects[i].w = user_rects[i].Width;
        pack_rects[i].h = user_rects[i].Height;
    }
    stbrp_pack_rects(pack_context, &pack_rects[0], pack_rects.Size);
    for (int i = 0; i < pack_rects.Size; i++)
        if (pack_rects[i].was_packed)
        {
            user_rects[i].X = pack_rects[i].x;
            user_rects[i].Y = pack_rects[i].y;
            IM_ASSERT(pack_rects[i].w == user_rects[i].Width && pack_rects[i].h == user_rects[i].Height);
            atlas->TexHeight = ImMax(atlas->TexHeight, pack_rects[i].y + pack_rects[i].h);
        }
}

void ImFontAtlasBuildRender1bppRectFromString(ImFontAtlas* atlas, int x, int y, int w, int h, const char* in_str, char in_marker_char, unsigned char in_marker_pixel_value)
{
    IM_ASSERT(x >= 0 && x + w <= atlas->TexWidth);
    IM_ASSERT(y >= 0 && y + h <= atlas->TexHeight);
    unsigned char* out_pixel = atlas->TexPixelsAlpha8 + x + (y * atlas->TexWidth);
    for (int off_y = 0; off_y < h; off_y++, out_pixel += atlas->TexWidth, in_str += w)
        for (int off_x = 0; off_x < w; off_x++)
            out_pixel[off_x] = (in_str[off_x] == in_marker_char) ? in_marker_pixel_value : 0x00;
}

static void ImFontAtlasBuildRenderDefaultTexData(ImFontAtlas* atlas)
{
    ImFontAtlasCustomRect* r = atlas->GetCustomRectByIndex(atlas->PackIdMouseCursors);
    IM_ASSERT(r->IsPacked());

    const int w = atlas->TexWidth;
    if (!(atlas->Flags & ImFontAtlasFlags_NoMouseCursors))
    {
        // Render/copy pixels
        IM_ASSERT(r->Width == FONT_ATLAS_DEFAULT_TEX_DATA_W * 2 + 1 && r->Height == FONT_ATLAS_DEFAULT_TEX_DATA_H);
        const int x_for_white = r->X;
        const int x_for_black = r->X + FONT_ATLAS_DEFAULT_TEX_DATA_W + 1;
        ImFontAtlasBuildRender1bppRectFromString(atlas, x_for_white, r->Y, FONT_ATLAS_DEFAULT_TEX_DATA_W, FONT_ATLAS_DEFAULT_TEX_DATA_H, FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS, '.', 0xFF);
        ImFontAtlasBuildRender1bppRectFromString(atlas, x_for_black, r->Y, FONT_ATLAS_DEFAULT_TEX_DATA_W, FONT_ATLAS_DEFAULT_TEX_DATA_H, FONT_ATLAS_DEFAULT_TEX_DATA_PIXELS, 'X', 0xFF);
    }
    else
    {
        // Render 4 white pixels
        IM_ASSERT(r->Width == 2 && r->Height == 2);
        const int offset = (int)r->X + (int)r->Y * w;
        atlas->TexPixelsAlpha8[offset] = atlas->TexPixelsAlpha8[offset + 1] = atlas->TexPixelsAlpha8[offset + w] = atlas->TexPixelsAlpha8[offset + w + 1] = 0xFF;
    }
    atlas->TexUvWhitePixel = ImVec2((r->X + 0.5f) * atlas->TexUvScale.x, (r->Y + 0.5f) * atlas->TexUvScale.y);
}

static void ImFontAtlasBuildRenderLinesTexData(ImFontAtlas* atlas)
{
    if (atlas->Flags & ImFontAtlasFlags_NoBakedLines)
        return;

    // This generates a triangular shape in the texture, with the various line widths stacked on top of each other to allow interpolation between them
    ImFontAtlasCustomRect* r = atlas->GetCustomRectByIndex(atlas->PackIdLines);
    IM_ASSERT(r->IsPacked());
    for (unsigned int n = 0; n < IM_DRAWLIST_TEX_LINES_WIDTH_MAX + 1; n++) // +1 because of the zero-width row
    {
        // Each line consists of at least two empty pixels at the ends, with a line of solid pixels in the middle
        unsigned int y = n;
        unsigned int line_width = n;
        unsigned int pad_left = (r->Width - line_width) / 2;
        unsigned int pad_right = r->Width - (pad_left + line_width);

        // Write each slice
        IM_ASSERT(pad_left + line_width + pad_right == r->Width && y < r->Height); // Make sure we're inside the texture bounds before we start writing pixels
        unsigned char* write_ptr = &atlas->TexPixelsAlpha8[r->X + ((r->Y + y) * atlas->TexWidth)];
        memset(write_ptr, 0x00, pad_left);
        memset(write_ptr + pad_left, 0xFF, line_width);
        memset(write_ptr + pad_left + line_width, 0x00, pad_right);

        // Calculate UVs for this line
        ImVec2 uv0 = ImVec2((float)(r->X + pad_left - 1), (float)(r->Y + y)) * atlas->TexUvScale;
        ImVec2 uv1 = ImVec2((float)(r->X + pad_left + line_width + 1), (float)(r->Y + y + 1)) * atlas->TexUvScale;
        float half_v = (uv0.y + uv1.y) * 0.5f; // Calculate a constant V in the middle of the row to avoid sampling artifacts
        atlas->TexUvLines[n] = ImVec4(uv0.x, half_v, uv1.x, half_v);
    }
}

// Note: this is called / shared by both the stb_truetype and the FreeType builder
void ImFontAtlasBuildInit(ImFontAtlas* atlas)
{
    // Register texture region for mouse cursors or standard white pixels
    if (atlas->PackIdMouseCursors < 0)
    {
        if (!(atlas->Flags & ImFontAtlasFlags_NoMouseCursors))
            atlas->PackIdMouseCursors = atlas->AddCustomRectRegular(FONT_ATLAS_DEFAULT_TEX_DATA_W * 2 + 1, FONT_ATLAS_DEFAULT_TEX_DATA_H);
        else
            atlas->PackIdMouseCursors = atlas->AddCustomRectRegular(2, 2);
    }

    // Register texture region for thick lines
    // The +2 here is to give space for the end caps, whilst height +1 is to accommodate the fact we have a zero-width row
    if (atlas->PackIdLines < 0)
    {
        if (!(atlas->Flags & ImFontAtlasFlags_NoBakedLines))
            atlas->PackIdLines = atlas->AddCustomRectRegular(IM_DRAWLIST_TEX_LINES_WIDTH_MAX + 2, IM_DRAWLIST_TEX_LINES_WIDTH_MAX + 1);
    }
}

// This is called/shared by both the stb_truetype and the FreeType builder.
void ImFontAtlasBuildFinish(ImFontAtlas* atlas)
{
    // Render into our custom data blocks
    IM_ASSERT(atlas->TexPixelsAlpha8 != NULL);
    ImFontAtlasBuildRenderDefaultTexData(atlas);
    ImFontAtlasBuildRenderLinesTexData(atlas);

    // Register custom rectangle glyphs
    for (int i = 0; i < atlas->CustomRects.Size; i++)
    {
        const ImFontAtlasCustomRect* r = &atlas->CustomRects[i];
        if (r->Font == NULL || r->GlyphID == 0)
            continue;

        // Will ignore ImFontConfig settings: GlyphMinAdvanceX, GlyphMinAdvanceY, GlyphExtraSpacing, PixelSnapH
        IM_ASSERT(r->Font->ContainerAtlas == atlas);
        ImVec2 uv0, uv1;
        atlas->CalcCustomRectUV(r, &uv0, &uv1);
        r->Font->AddGlyph(NULL, (ImWchar)r->GlyphID, r->GlyphOffset.x, r->GlyphOffset.y, r->GlyphOffset.x + r->Width, r->GlyphOffset.y + r->Height, uv0.x, uv0.y, uv1.x, uv1.y, r->GlyphAdvanceX);
    }

    // Build all fonts lookup tables
    for (int i = 0; i < atlas->Fonts.Size; i++)
        if (atlas->Fonts[i]->DirtyLookupTables)
            atlas->Fonts[i]->BuildLookupTable();

    // Ellipsis character is required for rendering elided text. We prefer using U+2026 (horizontal ellipsis).
    // However some old fonts may contain ellipsis at U+0085. Here we auto-detect most suitable ellipsis character.
    // FIXME: Also note that 0x2026 is currently seldom included in our font ranges. Because of this we are more likely to use three individual dots.
    for (int i = 0; i < atlas->Fonts.size(); i++)
    {
        ImFont* font = atlas->Fonts[i];
        if (font->EllipsisChar != (ImWchar)-1)
            continue;
        const ImWchar ellipsis_variants[] = { (ImWchar)0x2026, (ImWchar)0x0085 };
        for (int j = 0; j < IM_ARRAYSIZE(ellipsis_variants); j++)
            if (font->FindGlyphNoFallback(ellipsis_variants[j]) != NULL) // Verify glyph exists
            {
                font->EllipsisChar = ellipsis_variants[j];
                break;
            }
    }
}

// Retrieve list of range (2 int per range, values are inclusive)
const ImWchar*  ImFontAtlas::GetGlyphRangesDefault()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesKorean()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x3131, 0x3163, // Korean alphabets
        0xAC00, 0xD7A3, // Korean characters
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesChineseFull()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2000, 0x206F, // General Punctuation
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF, // Half-width characters
        0x4e00, 0x9FAF, // CJK Ideograms
        0,
    };
    return &ranges[0];
}

static void UnpackAccumulativeOffsetsIntoRanges(int base_codepoint, const short* accumulative_offsets, int accumulative_offsets_count, ImWchar* out_ranges)
{
    for (int n = 0; n < accumulative_offsets_count; n++, out_ranges += 2)
    {
        out_ranges[0] = out_ranges[1] = (ImWchar)(base_codepoint + accumulative_offsets[n]);
        base_codepoint += accumulative_offsets[n];
    }
    out_ranges[0] = 0;
}

//-------------------------------------------------------------------------
// [SECTION] ImFontAtlas glyph ranges helpers
//-------------------------------------------------------------------------

const ImWchar*  ImFontAtlas::GetGlyphRangesChineseSimplifiedCommon()
{
    // Store 2500 regularly used characters for Simplified Chinese.
    // Sourced from https://zh.wiktionary.org/wiki/%E9%99%84%E5%BD%95:%E7%8E%B0%E4%BB%A3%E6%B1%89%E8%AF%AD%E5%B8%B8%E7%94%A8%E5%AD%97%E8%A1%A8
    // This table covers 97.97% of all characters used during the month in July, 1987.
    // You can use ImFontGlyphRangesBuilder to create your own ranges derived from this, by merging existing ranges or adding new characters.
    // (Stored as accumulative offsets from the initial unicode codepoint 0x4E00. This encoding is designed to helps us compact the source code size.)
    static unsigned short ChinenseCommonMat[] = { 0x4e00,0x4e59,0x4e8c,0x5341,0x4e01,0x5382,0x4e03,0x535c,0x516b,0x4eba,0x5165,0x4e42,0x513f,0x4e5d,0x5315,
        0x51e0,0x5201,0x4e86,0x4e43,0x5200,0x529b,0x53c8,0x4e5c,0x4e09,0x5e72,0x4e8d,0x4e8e,0x4e8f,0x58eb,0x571f,0x5de5,0x624d,0x4e0b,0x5bf8,0x4e08,0x5927,
        0x5140,0x4e0e,0x4e07,0x5f0b,0x4e0a,0x5c0f,0x53e3,0x5c71,0x5dfe,0x5343,0x4e5e,0x5ddd,0x4ebf,0x5f73,0x4e2a,0x4e48,0x4e45,0x52fa,0x4e38,0x5915,0x51e1,
        0x53ca,0x5e7f,0x4ea1,0x95e8,0x4e2b,0x4e49,0x4e4b,0x5c38,0x5df2,0x5df3,0x5f13,0x5df1,0x536b,0x5b51,0x5b50,0x5b53,0x4e5f,0x5973,0x98de,0x5203,0x4e60,
        0x53c9,0x9a6c,0x4e61,0x5e7a,0x4e30,0x738b,0x4e95,0x5f00,0x4e93,0x592b,0x5929,0x5143,0x65e0,0x97e6,0x4e91,0x4e13,0x4e10,0x624e,0x5eff,0x827a,0x6728,
        0x4e94,0x652f,0x5385,0x5345,0x4e0d,0x4ec4,0x592a,0x72ac,0x533a,0x5386,0x53cb,0x6b79,0x5c24,0x5339,0x5384,0x8f66,0x5de8,0x7259,0x5c6f,0x6208,0x6bd4,
        0x4e92,0x5207,0x74e6,0x6b62,0x5c11,0x66f0,0x65e5,0x4e2d,0x8d1d,0x5185,0x6c34,0x5188,0x89c1,0x624b,0x5348,0x725b,0x6bdb,0x6c14,0x58ec,0x5347,0x592d,
        0x957f,0x4ec1,0x4ec3,0x4ec0,0x7247,0x4ec6,0x4ec9,0x5316,0x4ec7,0x5e01,0x4ec2,0x4ecd,0x4ec5,0x65a4,0x722a,0x53cd,0x516e,0x5208,0x4ecb,0x7236,0x723b,
        0x4ece,0x4ed1,0x4eca,0x51f6,0x5206,0x4e4f,0x516c,0x4ed3,0x6708,0x6c0f,0x52ff,0x98ce,0x6b20,0x4e39,0x5300,0x4e4c,0x52fe,0x6bb3,0x51e4,0x535e,0x516d,
        0x6587,0x4ea2,0x65b9,0x95e9,0x706b,0x4e3a,0x6597,0x5fc6,0x8ba1,0x8ba2,0x6237,0x8ba3,0x8ba4,0x8ba5,0x5197,0x5fc3,0x5c39,0x5c3a,0x592c,0x5f15,0x4e11,
        0x723f,0x5df4,0x5b54,0x961f,0x529e,0x4ee5,0x5141,0x9093,0x4e88,0x529d,0x53cc,0x4e66,0x6bcb,0x5e7b,0x7389,0x520a,0x672b,0x672a,0x793a,0x51fb,0x9097,
        0x620b,0x6253,0x5de7,0x6b63,0x6251,0x5349,0x6252,0x909b,0x529f,0x6254,0x53bb,0x7518,0x4e16,0x827e,0x827d,0x53e4,0x8282,0x827f,0x672c,0x672f,0x672d,
        0x53ef,0x53f5,0x531d,0x4e19,0x5de6,0x5389,0x4e15,0x77f3,0x53f3,0x5e03,0x592f,0x9f99,0x620a,0x5e73,0x706d,0x8f67,0x4e1c,0x531c,0x52a2,0x5361,0x5317,
        0x5360,0x51f8,0x5362,0x4e1a,0x65e7,0x5e05,0x5f52,0x76ee,0x65e6,0x4e14,0x53ee,0x53f6,0x7532,0x7533,0x53f7,0x7535,0x7530,0x7531,0x535f,0x53ed,0x53ea,
        0x592e,0x53f2,0x53f1,0x53fd,0x5144,0x53fc,0x53e9,0x53eb,0x53fb,0x53e8,0x53e6,0x53f9,0x5189,0x76bf,0x51f9,0x56da,0x56db,0x751f,0x5931,0x77e2,0x6c15,
        0x4e4d,0x79be,0x4ee8,0x4ed5,0x4e18,0x4ed8,0x4ed7,0x4ee3,0x4ed9,0x4edf,0x4ee1,0x4eeb,0x4f0b,0x4eec,0x4eea,0x767d,0x4ed4,0x4ed6,0x4ede,0x65a5,0x536e,
        0x74dc,0x4e4e,0x4e1b,0x4ee4,0x7528,0x7529,0x5370,0x6c10,0x4e50,0x5c14,0x53e5,0x5306,0x72b0,0x518c,0x536f,0x72af,0x5916,0x5904,0x51ac,0x9e1f,0x52a1,
        0x520d,0x5305,0x9965,0x4e3b,0x5e02,0x5e80,0x909d,0x7acb,0x51af,0x9099,0x7384,0x95ea,0x5170,0x534a,0x6c40,0x6c41,0x6c47,0x5934,0x6c48,0x6c49,0x5fc9,
        0x5b81,0x7a74,0x5b84,0x5b83,0x8ba6,0x8ba7,0x8ba8,0x5199,0x8ba9,0x793c,0x8baa,0x8bab,0x8bad,0x5fc5,0x8bae,0x8baf,0x8bb0,0x6c38,0x53f8,0x5c3b,0x5c3c,
        0x6c11,0x5f17,0x5f18,0x9622,0x51fa,0x9621,0x8fbd,0x5976,0x5974,0x5c15,0x52a0,0x53ec,0x76ae,0x8fb9,0x5b55,0x53d1,0x5723,0x5bf9,0x5f01,0x53f0,0x77db,
        0x7ea0,0x9a6d,0x6bcd,0x5e7c,0x4e1d,0x5321,0x8012,0x90a6,0x738e,0x7391,0x5f0f,0x8fc2,0x5211,0x90a2,0x620e,0x52a8,0x5729,0x572c,0x572d,0x625b,0x5bfa,
        0x5409,0x6263,0x6266,0x572a,0x8003,0x6258,0x5733,0x8001,0x573e,0x5de9,0x6267,0x6269,0x5739,0x626a,0x626b,0x572f,0x572e,0x5730,0x626c,0x573a,0x8033,
        0x828b,0x828f,0x5171,0x828a,0x828d,0x82a8,0x8284,0x8292,0x4e9a,0x829d,0x828e,0x8291,0x8297,0x673d,0x6734,0x673a,0x6743,0x8fc7,0x4e98,0x81e3,0x540f,
        0x518d,0x534f,0x897f,0x538b,0x538c,0x538d,0x620c,0x5728,0x767e,0x6709,0x5b58,0x800c,0x9875,0x5320,0x5938,0x593a,0x593c,0x7070,0x8fbe,0x620d,0x5c25,
        0x5217,0x6b7b,0x6210,0x5939,0x5937,0x8f68,0x90aa,0x5c27,0x5212,0x8fc8,0x6bd5,0x81f3,0x6b64,0x4e69,0x8d1e,0x5e08,0x5c18,0x5c16,0x52a3,0x5149,0x5f53,
        0x5401,0x65e9,0x5410,0x5413,0x65ef,0x66f3,0x866b,0x66f2,0x56e2,0x540c,0x5415,0x540a,0x5403,0x56e0,0x5438,0x5417,0x5406,0x5c7f,0x5c79,0x5c8c,0x5e06,
        0x5c81,0x56de,0x5c82,0x5c7a,0x5219,0x521a,0x7f51,0x8089,0x51fc,0x56dd,0x56e1,0x9486,0x9487,0x5e74,0x6731,0x7f36,0x6c18,0x6c16,0x725d,0x5148,0x4e22,
        0x5ef7,0x820c,0x7af9,0x8fc1,0x4e54,0x8fc4,0x4f1f,0x4f20,0x4e52,0x4e53,0x4f11,0x4f0d,0x4f0e,0x4f0f,0x4f1b,0x4f18,0x81fc,0x4f22,0x4f10,0x4ef3,0x5ef6,
        0x4f64,0x4ef2,0x4ef5,0x4ef6,0x4efb,0x4f24,0x4f25,0x4ef7,0x4f26,0x4efd,0x4f27,0x534e,0x4ef0,0x4f09,0x4eff,0x4f19,0x4f2a,0x4f2b,0x81ea,0x4f0a,0x8840,
        0x5411,0x56df,0x4f3c,0x540e,0x884c,0x752a,0x821f,0x5168,0x4f1a,0x6740,0x5408,0x5146,0x4f01,0x6c46,0x6c3d,0x4f17,0x7237,0x4f1e,0x521b,0x5216,0x808c,
        0x808b,0x6735,0x6742,0x5919,0x5371,0x65ec,0x65ed,0x65ee,0x65e8,0x8d1f,0x72b4,0x520e,0x72b7,0x5308,0x72b8,0x821b,0x5404,0x540d,0x591a,0x51eb,0x4e89,
        0x90ac,0x8272,0x9967,0x51b1,0x58ee,0x51b2,0x5986,0x51b0,0x5e84,0x5e86,0x4ea6,0x5218,0x9f50,0x4ea4,0x6b21,0x8863,0x4ea7,0x51b3,0x4ea5,0x90a1,0x5145,
        0x5984,0x95ed,0x95ee,0x95ef,0x7f8a,0x5e76,0x5173,0x7c73,0x706f,0x5dde,0x6c57,0x6c61,0x6c5f,0x6c55,0x6c54,0x6c72,0x6c50,0x6c5b,0x6c5c,0x6c60,0x6c5d,
        0x6c64,0x6c4a,0x5fd6,0x5fcf,0x5fd9,0x5174,0x5b87,0x5b88,0x5b85,0x5b57,0x5b89,0x8bb2,0x8bb3,0x8bb4,0x519b,0x8bb5,0x8bb6,0x7941,0x8bb7,0x8bb8,0x8bb9,
        0x8bba,0x8bbc,0x519c,0x8bbd,0x8bbe,0x8bbf,0x8bc0,0x807f,0x5bfb,0x90a3,0x826e,0x53be,0x8fc5,0x5c3d,0x5bfc,0x5f02,0x5f1b,0x9631,0x962e,0x5b59,0x9635,
        0x9633,0x6536,0x962a,0x9636,0x9634,0x9632,0x4e1e,0x5978,0x5982,0x5981,0x5987,0x5983,0x597d,0x5979,0x5988,0x620f,0x7fbd,0x89c2,0x725f,0x6b22,0x4e70,
        0x7ea1,0x7ea2,0x7ea3,0x9a6e,0x7ea4,0x7ea5,0x9a6f,0x7ea8,0x7ea6,0x7ea7,0x7ea9,0x7eaa,0x9a70,0x7eab,0x5de1,0x5bff,0x7395,0x5f04,0x7399,0x9ea6,0x7396,
        0x739a,0x739b,0x5f62,0x8fdb,0x6212,0x541e,0x8fdc,0x8fdd,0x97e7,0x8fd0,0x6276,0x629a,0x575b,0x629f,0x6280,0x574f,0x6294,0x62a0,0x575c,0x6270,0x627c,
        0x62d2,0x627e,0x6279,0x626f,0x5740,0x8d70,0x6284,0x6c5e,0x575d,0x8d21,0x653b,0x8d64,0x573b,0x6298,0x6293,0x6273,0x5742,0x62a1,0x626e,0x62a2,0x627a,
        0x5b5d,0x574e,0x574d,0x5747,0x575e,0x6291,0x629b,0x6295,0x6283,0x575f,0x5751,0x6297,0x574a,0x6296,0x62a4,0x58f3,0x5fd7,0x626d,0x5757,0x6289,0x58f0,
        0x628a,0x62a5,0x62df,0x6292,0x5374,0x52ab,0x6bd0,0x8299,0x82ab,0x829c,0x82c7,0x90af,0x82b8,0x82be,0x82b0,0x82c8,0x82ca,0x82e3,0x82bd,0x82b7,0x82ae,
        0x82cb,0x82bc,0x82cc,0x82b1,0x82b9,0x82a5,0x82c1,0x82a9,0x82ac,0x82cd,0x82aa,0x82b4,0x82a1,0x829f,0x82c4,0x82b3,0x4e25,0x82ce,0x82a6,0x82af,0x52b3,
        0x514b,0x82ad,0x82cf,0x82e1,0x6746,0x675c,0x6760,0x6750,0x6751,0x6756,0x674c,0x674f,0x6749,0x5deb,0x6753,0x6781,0x6767,0x675e,0x674e,0x6768,0x6748,
        0x6c42,0x5fd1,0x5b5b,0x752b,0x5323,0x66f4,0x675f,0x543e,0x8c46,0x4e24,0x90b4,0x9149,0x4e3d,0x533b,0x8fb0,0x52b1,0x90b3,0x5426,0x8fd8,0x77f6,0x5941,
        0x8c55,0x5c2c,0x6b7c,0x6765,0x5fd2,0x8fde,0x6b24,0x8f69,0x8f6a,0x8f6b,0x8fd3,0x90b6,0x5fd0,0x8288,0x6b65,0x5364,0x5363,0x90ba,0x575a,0x8096,0x65f0,
        0x65f1,0x76ef,0x5448,0x65f6,0x5434,0x544b,0x52a9,0x53bf,0x91cc,0x5453,0x5446,0x5431,0x5420,0x5454,0x5455,0x56ed,0x5456,0x5443,0x65f7,0x56f4,0x5440,
        0x5428,0x65f8,0x5421,0x753a,0x8db3,0x866c,0x90ae,0x7537,0x56f0,0x5435,0x4e32,0x5459,0x5450,0x5457,0x5458,0x542c,0x541f,0x5429,0x545b,0x543b,0x5439,
        0x545c,0x542d,0x5423,0x5432,0x543c,0x9091,0x5427,0x56e4,0x522b,0x542e,0x5c8d,0x5e0f,0x5c90,0x5c96,0x5c88,0x5c97,0x5c98,0x5e10,0x5c91,0x5c9a,0x5155,
        0x8d22,0x56f5,0x56eb,0x9489,0x9488,0x948a,0x948b,0x948c,0x8fd5,0x6c19,0x6c1a,0x7261,0x544a,0x6211,0x4e71,0x5229,0x79c3,0x79c0,0x79c1,0x5c99,0x6bcf,
        0x4f5e,0x5175,0x90b1,0x4f30,0x4f53,0x4f55,0x4f50,0x4f3e,0x4f51,0x6538,0x4f46,0x4f38,0x4f43,0x4f5a,0x4f5c,0x4f2f,0x4f36,0x4f63,0x4f4e,0x4f60,0x4f5d,
        0x4f5f,0x4f4f,0x4f4d,0x4f34,0x4f57,0x8eab,0x7682,0x4f3a,0x4f5b,0x4f3d,0x56f1,0x8fd1,0x5f7b,0x5f79,0x5f77,0x8fd4,0x4f58,0x4f59,0x5e0c,0x4f65,0x5750,
        0x8c37,0x5b5a,0x59a5,0x8c78,0x542b,0x90bb,0x574c,0x5c94,0x809d,0x809f,0x809b,0x809a,0x8098,0x80a0,0x90b8,0x9f9f,0x7538,0x5942,0x514d,0x52ac,0x72c2,
        0x72b9,0x72c8,0x72c4,0x89d2,0x5220,0x72c3,0x72c1,0x9e20,0x6761,0x5f64,0x5375,0x7078,0x5c9b,0x90b9,0x5228,0x9968,0x8fce,0x9969,0x996a,0x996b,0x996c,
        0x996d,0x996e,0x7cfb,0x8a00,0x51bb,0x72b6,0x4ea9,0x51b5,0x4ea8,0x5e91,0x5e8a,0x5e8b,0x5e93,0x5e87,0x7594,0x7596,0x7597,0x541d,0x5e94,0x51b7,0x8fd9,
        0x5e90,0x5e8f,0x8f9b,0x8093,0x5f03,0x51b6,0x5fd8,0x95f0,0x95f1,0x95f2,0x95f3,0x95f4,0x95f5,0x95f6,0x95f7,0x7f8c,0x5224,0x5151,0x7076,0x707f,0x707c,
        0x7080,0x5f1f,0x6ca3,0x6c6a,0x6c85,0x6c84,0x6c90,0x6c9b,0x6c94,0x6c70,0x6ca4,0x6ca5,0x6c8c,0x6c98,0x6c8f,0x6c9a,0x6c99,0x6c69,0x6c68,0x6c6d,0x6c7d,
        0x6c83,0x6c82,0x6ca6,0x6c79,0x6c7e,0x6cdb,0x6ca7,0x6ca8,0x6c9f,0x6ca1,0x6c74,0x6c76,0x6c86,0x6ca9,0x6caa,0x6c88,0x6c89,0x6c81,0x6cd0,0x6003,0x5fee,
        0x6000,0x6004,0x5fe7,0x5fe1,0x5fe4,0x5ffe,0x6005,0x5ffb,0x5fea,0x6006,0x5fed,0x5ff1,0x5feb,0x5ff8,0x5b8c,0x5b8b,0x5b8f,0x7262,0x7a76,0x7a77,0x707e,
        0x826f,0x8bc1,0x8bc2,0x8bc3,0x542f,0x8bc4,0x8865,0x521d,0x793e,0x7940,0x7943,0x8bc5,0x8bc6,0x8bc8,0x8bc9,0x7f55,0x8bca,0x8bcb,0x8bcc,0x8bcd,0x8bce,
        0x8bcf,0x8bd0,0x8bd1,0x8bd2,0x541b,0x7075,0x5373,0x5c42,0x5c41,0x5c43,0x5c3f,0x5c3e,0x8fdf,0x5c40,0x6539,0x5f20,0x5fcc,0x9645,0x9646,0x963f,0x5b5c,
        0x9647,0x9648,0x963d,0x963b,0x963c,0x9644,0x5760,0x9640,0x9642,0x9649,0x598d,0x59a9,0x5993,0x59aa,0x59a3,0x5999,0x598a,0x5996,0x5997,0x59ca,0x59a8,
        0x59ab,0x5992,0x599e,0x59d2,0x59a4,0x52aa,0x90b5,0x52ad,0x5fcd,0x522d,0x52b2,0x752c,0x90b0,0x77e3,0x9e21,0x7eac,0x7ead,0x9a71,0x7eaf,0x7eb0,0x7eb1,
        0x7eb2,0x7eb3,0x7eb4,0x7eb5,0x9a73,0x7eb6,0x7eb7,0x7eb8,0x7eb9,0x7eba,0x7ebb,0x9a74,0x7ebd,0x7ebe,0x5949,0x73a9,0x73ae,0x73af,0x73a1,0x6b66,0x9752,
        0x8d23,0x73b0,0x73ab,0x73a0,0x73a2,0x73a5,0x8868,0x73a6,0x7519,0x76c2,0x5fdd,0x89c4,0x5326,0x62b9,0x5366,0x90bd,0x5769,0x5777,0x576f,0x62d3,0x5785,
        0x62e2,0x62d4,0x62a8,0x576a,0x62e3,0x62e4,0x62c8,0x576b,0x5786,0x5766,0x62c5,0x5764,0x62bc,0x62bb,0x62bd,0x62d0,0x62c3,0x62d6,0x62ca,0x8005,0x62cd,
        0x9876,0x577c,0x62c6,0x62ce,0x62e5,0x62b5,0x577b,0x62d8,0x52bf,0x62b1,0x62c4,0x5783,0x62c9,0x62e6,0x5e78,0x62cc,0x62e7,0x5768,0x576d,0x62bf,0x62c2,
        0x62d9,0x62db,0x5761,0x62ab,0x62e8,0x62e9,0x62da,0x62ac,0x62c7,0x5773,0x62d7,0x8035,0x5176,0x8036,0x53d6,0x8309,0x82f7,0x82e6,0x82ef,0x6614,0x82db,
        0x82e4,0x82e5,0x8302,0x830f,0x82f9,0x82eb,0x82f4,0x82dc,0x82d7,0x82f1,0x82d2,0x82d8,0x830c,0x82fb,0x82d3,0x831a,0x82df,0x8306,0x8311,0x82d1,0x82de,
        0x8303,0x8313,0x8314,0x8315,0x76f4,0x82e0,0x8300,0x8301,0x8304,0x82d5,0x830e,0x82d4,0x8305,0x6789,0x6797,0x679d,0x676f,0x67a2,0x67a5,0x67dc,0x6787,
        0x676a,0x6773,0x6798,0x67a7,0x6775,0x679a,0x67a8,0x6790,0x677f,0x679e,0x677e,0x67aa,0x67ab,0x6784,0x676d,0x678b,0x6770,0x8ff0,0x6795,0x677b,0x6777,
        0x677c,0x4e27,0x6216,0x753b,0x5367,0x4e8b,0x523a,0x67a3,0x96e8,0x5356,0x77f8,0x90c1,0x77fb,0x77fe,0x77fd,0x77ff,0x7800,0x7801,0x5395,0x5948,0x5233,
        0x5954,0x5947,0x5944,0x594b,0x6001,0x74ef,0x6b27,0x6bb4,0x5784,0x6b81,0x90cf,0x59bb,0x8f70,0x9877,0x8f6c,0x8f6d,0x65a9,0x8f6e,0x8f6f,0x5230,0x90c5,
        0x9e22,0x975e,0x53d4,0x6b67,0x80af,0x9f7f,0x4e9b,0x5353,0x864e,0x864f,0x80be,0x8d24,0x5c1a,0x76f1,0x65fa,0x5177,0x660a,0x6619,0x679c,0x5473,0x6772,
        0x6603,0x6606,0x56fd,0x54ce,0x5495,0x660c,0x5475,0x5482,0x7545,0x5478,0x6615,0x660e,0x6613,0x5499,0x6600,0x6602,0x65fb,0x6609,0x7085,0x5494,0x7540,
        0x866e,0x8fea,0x5178,0x56fa,0x5fe0,0x5480,0x5477,0x547b,0x9efe,0x5492,0x548b,0x5490,0x5471,0x547c,0x5464,0x549a,0x9e23,0x5486,0x549b,0x548f,0x5462,
        0x5484,0x5476,0x5496,0x5466,0x549d,0x5cb5,0x5ca2,0x5cb8,0x5ca9,0x5e16,0x7f57,0x5cbf,0x5cac,0x5cab,0x5e1c,0x5e19,0x5e15,0x5cad,0x5ca3,0x5cc1,0x523f,
        0x5cc2,0x8fe5,0x5cb7,0x5240,0x51ef,0x5e14,0x5cc4,0x6c93,0x8d25,0x8d26,0x8d29,0x8d2c,0x8d2d,0x8d2e,0x56f9,0x56fe,0x7f54,0x948d,0x948e,0x948f,0x9490,
        0x9493,0x9492,0x9494,0x9495,0x9497,0x90be,0x5236,0x77e5,0x8fed,0x6c1b,0x8fee,0x5782,0x7266,0x7267,0x7269,0x4e56,0x522e,0x79c6,0x548c,0x5b63,0x59d4,
        0x7afa,0x79c9,0x8fe4,0x4f73,0x4f8d,0x4f76,0x5cb3,0x4f6c,0x4f74,0x4f9b,0x4f7f,0x4f91,0x4f70,0x4f89,0x4f8b,0x4fa0,0x81fe,0x4fa5,0x7248,0x4f84,0x5cb1,
        0x4fa6,0x4fa3,0x4f97,0x4f83,0x4fa7,0x4f8f,0x51ed,0x4fa8,0x4fa9,0x4f7b,0x4f7e,0x4f69,0x8d27,0x4f88,0x4faa,0x4f7c,0x4f9d,0x4f6f,0x4fac,0x5e1b,0x5351,
        0x7684,0x8feb,0x961c,0x4f94,0x8d28,0x6b23,0x90c8,0x5f81,0x5f82,0x5f80,0x722c,0x5f7c,0x5f84,0x6240,0x820d,0x91d1,0x523d,0x90d0,0x5239,0x547d,0x80b4,
        0x90c4,0x65a7,0x6002,0x7238,0x91c7,0x7c74,0x89c5,0x53d7,0x4e73,0x8d2a,0x5ff5,0x8d2b,0x5fff,0x74ee,0x6217,0x80bc,0x80a4,0x670a,0x80ba,0x80a2,0x80bd,
        0x80b1,0x80ab,0x80bf,0x80ad,0x80c0,0x670b,0x80b7,0x80a1,0x80ae,0x80aa,0x80a5,0x670d,0x80c1,0x5468,0x5241,0x660f,0x8fe9,0x90c7,0x9c7c,0x5154,0x72c9,
        0x72d9,0x72ce,0x72d0,0x5ffd,0x72dd,0x72d7,0x72cd,0x72de,0x72d2,0x548e,0x5907,0x7099,0x67ad,0x996f,0x9970,0x9971,0x9972,0x9973,0x9974,0x51bd,0x53d8,
        0x4eac,0x4eab,0x51bc,0x5e9e,0x5e97,0x591c,0x5e99,0x5e9c,0x5e95,0x5e96,0x759f,0x75a0,0x759d,0x7599,0x759a,0x75a1,0x5242,0x5352,0x90ca,0x5156,0x5e9a,
        0x5e9f,0x51c0,0x59be,0x76f2,0x653e,0x65bc,0x523b,0x52be,0x80b2,0x6c13,0x95f8,0x95f9,0x90d1,0x5238,0x5377,0x5355,0x709c,0x70ac,0x7096,0x7092,0x709d,
        0x708a,0x7095,0x708e,0x7089,0x7094,0x6cab,0x6d45,0x6cd5,0x6cd4,0x6cc4,0x6cbd,0x6cad,0x6cb3,0x6cf7,0x6cbe,0x6cf8,0x6cae,0x6cea,0x6cb9,0x6cf1,0x6cc5,
        0x6cd7,0x6cca,0x6ce0,0x6cdc,0x6cfa,0x6cc3,0x6cbf,0x6cd6,0x6ce1,0x6ce8,0x6ce3,0x6ceb,0x6cee,0x6cde,0x6cb1,0x6cfb,0x6ccc,0x6cf3,0x6ce5,0x6cef,0x6cb8,
        0x6cd3,0x6cbc,0x6ce2,0x6cfc,0x6cfd,0x6cfe,0x6cbb,0x6014,0x602f,0x6019,0x6035,0x6016,0x6026,0x601b,0x600f,0x6027,0x600d,0x6015,0x601c,0x6029,0x602b,
        0x600a,0x603f,0x602a,0x6021,0x5b66,0x5b9d,0x5b97,0x5b9a,0x5b95,0x5ba0,0x5b9c,0x5ba1,0x5b99,0x5b98,0x7a7a,0x5e18,0x7a78,0x7a79,0x5b9b,0x5b9e,0x5b93,
        0x8bd3,0x8bd4,0x8bd5,0x90ce,0x8bd6,0x8bd7,0x8bd8,0x623e,0x80a9,0x623f,0x8bd9,0x623d,0x8bda,0x90d3,0x886c,0x886b,0x8869,0x7946,0x794e,0x7949,0x89c6,
        0x7948,0x8bdb,0x8bdc,0x8bdd,0x8bde,0x8bdf,0x8be0,0x8be1,0x8be2,0x8be3,0x8be4,0x8be5,0x8be6,0x8be7,0x8be8,0x8be9,0x5efa,0x8083,0x96b6,0x5f55,0x5e1a,
        0x5c49,0x5c45,0x5c4a,0x5237,0x9e24,0x5c48,0x5f27,0x5f25,0x5f26,0x627f,0x5b5f,0x964b,0x6215,0x964c,0x5b64,0x5b62,0x9655,0x4e9f,0x964d,0x51fd,0x9654,
        0x9650,0x537a,0x59b9,0x59d1,0x59d0,0x59b2,0x59af,0x59d3,0x59d7,0x59ae,0x59cb,0x5e11,0x5f29,0x5b65,0x9a7d,0x59c6,0x8671,0x8fe6,0x8fe2,0x9a7e,0x53c1,
        0x53c2,0x8fe8,0x8270,0x7ebf,0x7ec0,0x7ec1,0x7ec2,0x7ec3,0x9a75,0x7ec4,0x7ec5,0x7ec6,0x9a76,0x7ec7,0x9a77,0x9a78,0x9a79,0x7ec8,0x7ec9,0x9a7a,0x9a7b,
        0x7eca,0x9a7c,0x7ecb,0x7ecc,0x7ecd,0x9a7f,0x7ece,0x7ecf,0x9a80,0x8d2f,0x753e,0x7809,0x8014,0x5951,0x8d30,0x594f,0x6625,0x5e2e,0x73cf,0x73d0,0x73c2,
        0x73d1,0x73b7,0x73b3,0x73c0,0x9878,0x73cd,0x73b2,0x73ca,0x73c9,0x73c8,0x73bb,0x6bd2,0x578b,0x97e8,0x62ed,0x6302,0x5c01,0x6301,0x62ee,0x62f7,0x62f1,
        0x57ad,0x631d,0x57a3,0x9879,0x57ae,0x630e,0x57af,0x631e,0x57ce,0x631f,0x6320,0x57a4,0x653f,0x8d74,0x8d75,0x8d73,0x8d32,0x57b1,0x6321,0x62fd,0x578c,
        0x54c9,0x57b2,0x633a,0x62ec,0x6322,0x57cf,0x90dd,0x578d,0x57a7,0x57a2,0x62f4,0x62fe,0x6311,0x579b,0x6307,0x57ab,0x6323,0x6324,0x5793,0x579f,0x62fc,
        0x579e,0x6316,0x6309,0x6325,0x6326,0x632a,0x57a0,0x62ef,0x62f6,0x67d0,0x751a,0x8346,0x8338,0x9769,0x831c,0x832c,0x8350,0x8359,0x5df7,0x835a,0x8351,
        0x8d33,0x835b,0x835c,0x8308,0x5e26,0x8349,0x8327,0x833c,0x8392,0x8335,0x8334,0x8331,0x839b,0x835e,0x832f,0x834f,0x8347,0x8343,0x835f,0x8336,0x8340,
        0x8317,0x8360,0x832d,0x8328,0x8352,0x57a9,0x8333,0x832b,0x8361,0x8363,0x8364,0x8365,0x8366,0x8367,0x8368,0x831b,0x6545,0x8369,0x80e1,0x836a,0x836b,
        0x8339,0x8354,0x5357,0x836c,0x836d,0x836f,0x67f0,0x6807,0x6808,0x67d1,0x67af,0x6809,0x67ef,0x67c4,0x67d8,0x680a,0x67e9,0x67b0,0x680b,0x680c,0x76f8,
        0x67e5,0x67d9,0x67b5,0x67da,0x67b3,0x67de,0x67cf,0x67dd,0x6800,0x67c3,0x67e2,0x680e,0x67b8,0x6805,0x67f3,0x67f1,0x67ff,0x680f,0x67c8,0x67e0,0x67c1,
        0x67b7,0x67fd,0x6811,0x52c3,0x524c,0x90da,0x5245,0x8981,0x914a,0x90e6,0x67ec,0x54b8,0x5a01,0x6b6a,0x752d,0x7814,0x7816,0x5398,0x7817,0x539a,0x7811,
        0x7818,0x7812,0x780c,0x7802,0x6cf5,0x781a,0x65ab,0x782d,0x781c,0x780d,0x9762,0x8010,0x800d,0x594e,0x8037,0x7275,0x9e25,0x867a,0x6b8b,0x6b82,0x6b83,
        0x6b87,0x6b84,0x6b86,0x8f71,0x8f72,0x8f73,0x8f74,0x8f75,0x8f76,0x8f77,0x8f78,0x8f79,0x8f7a,0x8f7b,0x9e26,0x867f,0x7686,0x6bd6,0x97ed,0x80cc,0x6218,
        0x89c7,0x70b9,0x8650,0x4e34,0x89c8,0x7ad6,0x5c1c,0x7701,0x524a,0x5c1d,0x54d0,0x6627,0x7704,0x770d,0x76f9,0x662f,0x90e2,0x7707,0x770a,0x76fc,0x7728,
        0x663d,0x7708,0x54c7,0x54ad,0x54c4,0x54d1,0x663e,0x5192,0x6620,0x79ba,0x54c2,0x661f,0x6628,0x54b4,0x66f7,0x6634,0x54a7,0x6631,0x6635,0x54a6,0x54d3,
        0x662d,0x54d4,0x754e,0x754f,0x6bd7,0x8db4,0x5472,0x80c4,0x80c3,0x8d35,0x754b,0x7548,0x754c,0x8679,0x867e,0x867c,0x867b,0x8681,0x601d,0x8682,0x76c5,
        0x54a3,0x867d,0x54c1,0x54bd,0x9a82,0x54d5,0x5250,0x90e7,0x52cb,0x54bb,0x54d7,0x56ff,0x54b1,0x54bf,0x54cd,0x54cc,0x54d9,0x54c8,0x54da,0x54af,0x54c6,
        0x54ac,0x54b3,0x54a9,0x54aa,0x54a4,0x54dd,0x54ea,0x54cf,0x54de,0x54df,0x5cd9,0x70ad,0x5ce1,0x5ce3,0x7f58,0x5e27,0x7f5a,0x5cd2,0x5ce4,0x5ccb,0x5ce5,
        0x5ce7,0x5e21,0x8d31,0x8d34,0x8d36,0x8d3b,0x9aa8,0x5e7d,0x9498,0x9499,0x949a,0x949b,0x949d,0x949e,0x949f,0x94a1,0x94a0,0x94a2,0x94a3,0x94a4,0x94a5,
        0x94a6,0x94a7,0x94a8,0x94a9,0x94aa,0x94ab,0x94ac,0x94ad,0x94ae,0x94af,0x5378,0x7f38,0x62dc,0x770b,0x77e9,0x77e7,0x6be1,0x6c21,0x6c1f,0x6c22,0x726f,
        0x600e,0x90dc,0x7272,0x9009,0x9002,0x79d5,0x79d2,0x9999,0x79cd,0x79ed,0x79cb,0x79d1,0x91cd,0x590d,0x7afd,0x7aff,0x7b08,0x7b03,0x4fe6,0x6bb5,0x4fe8,
        0x4fc5,0x4fbf,0x4fe9,0x4fea,0x53df,0x57a1,0x8d37,0x726e,0x987a,0x4fee,0x4fcf,0x4fe3,0x4fda,0x4fdd,0x4fdc,0x4fc3,0x4fc4,0x4fd0,0x4fae,0x4fed,0x4fd7,
        0x4fd8,0x4fe1,0x7687,0x6cc9,0x7688,0x9b3c,0x4fb5,0x79b9,0x4faf,0x8ffd,0x4fd1,0x4fdf,0x4fca,0x76fe,0x9005,0x5f85,0x5f8a,0x5f87,0x5f89,0x884d,0x5f8b,
        0x5f88,0x987b,0x8222,0x8223,0x53d9,0x4fde,0x5f07,0x90d7,0x5251,0x9003,0x4fce,0x537b,0x7230,0x90db,0x98df,0x74f4,0x76c6,0x80da,0x80e7,0x80e8,0x80e9,
        0x80ea,0x80c6,0x80db,0x80c2,0x80dc,0x80d9,0x80cd,0x80d7,0x80dd,0x6710,0x80de,0x80d6,0x8109,0x80eb,0x80ce,0x9e28,0x530d,0x52c9,0x72e8,0x72ed,0x72ee,
        0x72ec,0x72ef,0x72f0,0x72e1,0x98d0,0x98d1,0x72e9,0x72f1,0x72e0,0x72f2,0x8a07,0x8a04,0x9004,0x661d,0x8d38,0x6028,0x6025,0x9975,0x9976,0x8680,0x9977,
        0x9978,0x9979,0x997a,0x997b,0x80e4,0x997c,0x5ce6,0x5f2f,0x5b6a,0x5a08,0x5c06,0x5956,0x54c0,0x4ead,0x4eae,0x5ea4,0x5ea6,0x5f08,0x5955,0x8ff9,0x5ead,
        0x5ea5,0x75ac,0x75a3,0x75a5,0x75ad,0x75ae,0x75af,0x75ab,0x75a2,0x75a4,0x5ea0,0x54a8,0x59ff,0x4eb2,0x7ad1,0x97f3,0x5f66,0x98d2,0x5e1d,0x65bd,0x95fa,
        0x95fb,0x95fc,0x95fd,0x95fe,0x95ff,0x9600,0x9601,0x9602,0x5dee,0x517b,0x7f8e,0x7f91,0x59dc,0x8ff8,0x53db,0x9001,0x7c7b,0x7c7c,0x8ff7,0x7c7d,0x5a04,
        0x524d,0x914b,0x9996,0x9006,0x5179,0x603b,0x70b3,0x70bb,0x70bc,0x709f,0x70bd,0x70af,0x70b8,0x70c0,0x70c1,0x70ae,0x70b7,0x70ab,0x70c2,0x70c3,0x5243,
        0x6d3c,0x6d01,0x6d31,0x6d2a,0x6d39,0x6d12,0x6d27,0x6d0c,0x6d43,0x67d2,0x6d47,0x6cda,0x6d48,0x6d49,0x6d4a,0x6d1e,0x6d07,0x6d04,0x6d4b,0x6d19,0x6d17,
        0x6d3b,0x6d11,0x6d8e,0x6d0e,0x6d2b,0x6d3e,0x6d4d,0x6d3d,0x6d2e,0x67d3,0x6d35,0x6d1a,0x6d3a,0x6d1b,0x6d4f,0x6d4e,0x6d28,0x6d50,0x6d0b,0x6d34,0x6d23,
        0x6d32,0x6d51,0x6d52,0x6d53,0x6d25,0x6d54,0x6d55,0x6d33,0x6078,0x6043,0x6052,0x6079,0x6062,0x604d,0x606b,0x607a,0x607b,0x606c,0x6064,0x6070,0x6042,
        0x606a,0x607c,0x607d,0x6068,0x4e3e,0x89c9,0x5ba3,0x5ba6,0x5ba5,0x5bac,0x5ba4,0x5bab,0x5baa,0x7a81,0x7a7f,0x7a80,0x7a83,0x5ba2,0x8beb,0x51a0,0x8bec,
        0x8bed,0x6241,0x6243,0x8886,0x8872,0x887d,0x8884,0x887f,0x8882,0x795b,0x795c,0x7953,0x7956,0x795e,0x795d,0x795a,0x8bee,0x7957,0x7962,0x7960,0x8bef,
        0x8bf0,0x8bf1,0x8bf2,0x8bf3,0x9e29,0x8bf4,0x6636,0x8bf5,0x90e1,0x57a6,0x9000,0x65e2,0x5c4b,0x663c,0x54ab,0x5c4f,0x5c4e,0x5f2d,0x8d39,0x9661,0x900a,
        0x7241,0x7709,0x80e5,0x5b69,0x965b,0x965f,0x9667,0x9668,0x9664,0x9669,0x9662,0x5a03,0x59de,0x59e5,0x5a05,0x59e8,0x5a06,0x59fb,0x59dd,0x5a07,0x59da,
        0x59fd,0x59e3,0x59d8,0x59f9,0x5a1c,0x6012,0x67b6,0x8d3a,0x76c8,0x603c,0x7fbf,0x52c7,0x70b1,0x6020,0x7678,0x86a4,0x67d4,0x77dc,0x5792,0x7ed1,0x7ed2,
        0x7ed3,0x7ed4,0x9a81,0x7ed5,0x9a84,0x9a85,0x7ed7,0x7ed8,0x7ed9,0x7eda,0x5f56,0x7edb,0x7edc,0x9a86,0x7edd,0x7ede,0x9a87,0x7edf,0x9a88,0x8015,0x8018,
        0x8016,0x8017,0x8019,0x8273,0x6308,0x605d,0x6cf0,0x79e6,0x73e5,0x73d9,0x987c,0x73f0,0x73e0,0x73fd,0x73e9,0x73e7,0x73e3,0x73de,0x7424,0x73ed,0x73f2,
        0x6556,0x7d20,0x533f,0x8695,0x987d,0x76cf,0x532a,0x605a,0x635e,0x683d,0x6355,0x57d4,0x57c2,0x6342,0x632f,0x8f7d,0x8d76,0x8d77,0x76d0,0x634e,0x634d,
        0x57d5,0x634f,0x57d8,0x57cb,0x6349,0x6346,0x6350,0x57da,0x57d9,0x635f,0x8881,0x6339,0x634c,0x90fd,0x54f2,0x901d,0x8006,0x8004,0x6361,0x632b,0x634b,
        0x57d2,0x6362,0x633d,0x8d3d,0x631a,0x70ed,0x6050,0x6363,0x57b8,0x58f6,0x6343,0x6345,0x76cd,0x57c3,0x6328,0x803b,0x803f,0x803d,0x8042,0x83b0,0x831d,
        0x8378,0x8386,0x606d,0x83bd,0x83b1,0x83b2,0x83b3,0x83ab,0x83b4,0x83aa,0x8389,0x83a0,0x8393,0x8377,0x839c,0x8385,0x837c,0x83b6,0x83a9,0x837d,0x83b7,
        0x83b8,0x837b,0x8398,0x664b,0x6076,0x838e,0x839e,0x83b9,0x83a8,0x83ba,0x771f,0x8399,0x9e2a,0x83bc,0x6846,0x6886,0x6842,0x6854,0x6832,0x6833,0x90f4,
        0x6853,0x6816,0x6861,0x684e,0x6862,0x6844,0x6863,0x6850,0x6864,0x682a,0x6883,0x681d,0x6865,0x6855,0x6866,0x6841,0x6813,0x6867,0x6843,0x6845,0x6812,
        0x683c,0x6869,0x6821,0x6838,0x6837,0x681f,0x6849,0x6839,0x6829,0x9011,0x7d22,0x900b,0x5f67,0x54e5,0x901f,0x9b32,0x8c47,0x9017,0x6817,0x8d3e,0x9150,
        0x914e,0x914c,0x914d,0x914f,0x9026,0x7fc5,0x8fb1,0x5507,0x539d,0x5b6c,0x590f,0x781d,0x7839,0x7838,0x783a,0x7830,0x7827,0x7837,0x781f,0x783c,0x7825,
        0x783e,0x7823,0x7840,0x7834,0x7841,0x6067,0x539f,0x5957,0x525e,0x9010,0x783b,0x70c8,0x6b8a,0x6b89,0x987e,0x8f7c,0x8f7e,0x8f7f,0x8f80,0x8f81,0x8f82,
        0x8f83,0x9e2b,0x987f,0x8db8,0x6bd9,0x81f4,0x5255,0x9f80,0x67f4,0x684c,0x9e2c,0x8654,0x8651,0x76d1,0x7d27,0x900d,0x515a,0x772c,0x551b,0x901e,0x6652,
        0x665f,0x7729,0x7720,0x6653,0x7719,0x551d,0x54e7,0x54f3,0x54ee,0x5520,0x9e2d,0x6643,0x54fa,0x54fd,0x5514,0x6654,0x664c,0x6641,0x5254,0x664f,0x6656,
        0x6655,0x9e2e,0x8db5,0x8dbf,0x755b,0x868c,0x86a8,0x869c,0x868d,0x868b,0x86ac,0x7554,0x869d,0x86a7,0x86a3,0x868a,0x86aa,0x8693,0x54e8,0x5522,0x54e9,
        0x5703,0x54ed,0x5704,0x54e6,0x5523,0x550f,0x6069,0x76ce,0x5511,0x9e2f,0x5524,0x5501,0x54fc,0x5527,0x554a,0x5509,0x5506,0x5e31,0x5d02,0x5d03,0x7f61,
        0x7f62,0x7f5f,0x5ced,0x5ce8,0x5cea,0x5cf0,0x5706,0x89ca,0x5cfb,0x8d3c,0x8d3f,0x8d42,0x8d43,0x8d45,0x8d46,0x94b0,0x94b1,0x94b2,0x94b3,0x94b4,0x94b5,
        0x94b7,0x94b9,0x94ba,0x94bb,0x94bc,0x94bd,0x94be,0x94bf,0x94c0,0x94c1,0x94c2,0x94c3,0x94c4,0x94c5,0x94c6,0x94c8,0x94c9,0x94ca,0x94cb,0x94cc,0x94cd,
        0x94ce,0x771a,0x7f3a,0x6c29,0x6c24,0x6c26,0x6c27,0x6c28,0x6bea,0x7279,0x727a,0x9020,0x4e58,0x654c,0x8210,0x79e3,0x79eb,0x79e4,0x79df,0x79e7,0x79ef,
        0x76c9,0x79e9,0x79f0,0x79d8,0x900f,0x7b04,0x7b15,0x7b14,0x7b11,0x7b0a,0x7b2b,0x7b0f,0x7b0b,0x7b06,0x4ff8,0x5029,0x503a,0x4ff5,0x503b,0x501f,0x504c,
        0x503c,0x501a,0x4ffa,0x503e,0x5012,0x4ff3,0x4ff6,0x502c,0x500f,0x5018,0x4ff1,0x5021,0x5019,0x8d41,0x6041,0x502d,0x502a,0x4ffe,0x501c,0x96bc,0x96bd,
        0x501e,0x4fef,0x500d,0x5026,0x5013,0x500c,0x5025,0x81ec,0x5065,0x81ed,0x5c04,0x768b,0x8eac,0x606f,0x90eb,0x5028,0x5014,0x8844,0x9880,0x5f92,0x5f95,
        0x5f90,0x6bb7,0x8230,0x8228,0x8231,0x822c,0x822a,0x822b,0x74de,0x9014,0x62ff,0x91dc,0x8038,0x7239,0x8200,0x7231,0x8c7a,0x8c79,0x595a,0x9b2f,0x887e,
        0x9e30,0x9881,0x9882,0x7fc1,0x80ef,0x80f0,0x80f1,0x80f4,0x80ed,0x810d,0x810e,0x8106,0x8102,0x80f8,0x80f3,0x810f,0x8110,0x80f6,0x8111,0x80f2,0x80fc,
        0x6715,0x8112,0x80fa,0x8113,0x9e31,0x73ba,0x9c7d,0x9e32,0x901b,0x72f4,0x72f8,0x72f7,0x7301,0x72f3,0x7303,0x72fa,0x9016,0x72fc,0x537f,0x72fb,0x9022,
        0x6840,0x9e35,0x7559,0x8885,0x7722,0x9e33,0x76b1,0x997d,0x997f,0x9980,0x9981,0x51cc,0x51c7,0x51c4,0x683e,0x631b,0x604b,0x6868,0x6d46,0x8870,0x52cd,
        0x8877,0x9ad8,0x4eb3,0x90ed,0x5e2d,0x51c6,0x5ea7,0x810a,0x75c7,0x75b3,0x75b4,0x75c5,0x75bd,0x75b8,0x75be,0x75c4,0x658b,0x75b9,0x75c8,0x75bc,0x75b1,
        0x75b0,0x75c3,0x75c2,0x75b2,0x75c9,0x810a,0x6548,0x79bb,0x886e,0x7d0a,0x5510,0x51cb,0x9883,0x74f7,0x8d44,0x6063,0x51c9,0x7ad9,0x5256,0x7ade,0x90e8,
        0x65c1,0x65c6,0x65c4,0x65c5,0x65c3,0x755c,0x9603,0x9604,0x9605,0x9606,0x7f9e,0x7f94,0x6059,0x74f6,0x684a,0x62f3,0x6549,0x7c89,0x6599,0x7c91,0x76ca,
        0x517c,0x6714,0x90f8,0x70e4,0x70d8,0x70dc,0x70e6,0x70e7,0x70db,0x70df,0x70e8,0x70e9,0x70d9,0x70ca,0x5261,0x90ef,0x70ec,0x9012,0x6d9b,0x6d59,0x6d9d,
        0x6d61,0x6d66,0x6d91,0x6d6f,0x9152,0x6d9e,0x6d9f,0x6d89,0x5a11,0x6d88,0x6d85,0x6da0,0x6d5e,0x6d93,0x6da2,0x6da1,0x6d65,0x6d94,0x6d69,0x6d77,0x6d5c,
        0x6d82,0x6d60,0x6d74,0x6d6e,0x6da3,0x6d7c,0x6da4,0x6d41,0x6da6,0x6da7,0x6d95,0x6d63,0x6d6a,0x6d78,0x6da8,0x70eb,0x6da9,0x6d8c,0x6d98,0x6d5a,0x6096,
        0x609a,0x609f,0x60ad,0x6084,0x608d,0x609d,0x6083,0x6092,0x6094,0x60af,0x60a6,0x608c,0x60a2,0x609b,0x5bb3,0x5bbd,0x5bb8,0x5bb6,0x5bb5,0x5bb4,0x5bbe,
        0x7a8d,0x7a85,0x7a84,0x5bb9,0x7a88,0x525c,0x5bb0,0x6848,0x8bf7,0x6717,0x8bf8,0x8bf9,0x8bfa,0x8bfb,0x6245,0x8bfc,0x51a2,0x6247,0x8bfd,0x889c,0x88aa,
        0x8892,0x8896,0x8897,0x888d,0x88a2,0x88ab,0x88af,0x796f,0x7967,0x7965,0x8bfe,0x51a5,0x8bff,0x8c00,0x8c01,0x8c02,0x8c03,0x51a4,0x8c04,0x8c05,0x8c06,
        0x8c07,0x8c08,0x8c0a,0x5265,0x6073,0x5c55,0x5267,0x5c51,0x5c50,0x5c59,0x5f31,0x9675,0x966c,0x52d0,0x5958,0x758d,0x7242,0x86a9,0x795f,0x9672,0x9674,
        0x9676,0x9677,0x966a,0x70dd,0x59ec,0x5a20,0x5a31,0x5a0c,0x5a09,0x5a1f,0x5a32,0x6055,0x5a25,0x5a29,0x5a34,0x5a23,0x5a18,0x5a13,0x5a40,0x782e,0x54ff,
        0x755a,0x901a,0x80fd,0x96be,0x9021,0x9884,0x6851,0x525f,0x7ee0,0x9a8a,0x7ee1,0x9a8b,0x7ee2,0x7ee3,0x9a8c,0x7ee4,0x7ee5,0x7ee6,0x9a8d,0x7ee7,0x7ee8,
        0x9a8e,0x9a8f,0x9095,0x70dd,0x9e36,0x5f57,0x801c,0x7118,0x8202,0x740e,0x7403,0x740f,0x7410,0x7406,0x7407,0x9eb8,0x7409,0x7405,0x6367,0x63ad,0x5835,
        0x63f6,0x63aa,0x63cf,0x57f4,0x57df,0x637a,0x638e,0x57fc,0x63a9,0x57ef,0x6377,0x636f,0x6392,0x7109,0x6389,0x63b3,0x63b4,0x57f8,0x580c,0x6376,0x8d66,
        0x8d67,0x63a8,0x5806,0x636d,0x57e0,0x6662,0x6380,0x9035,0x6388,0x637b,0x57dd,0x580b,0x6559,0x580d,0x638f,0x6390,0x63ac,0x9e37,0x63a0,0x6382,0x6396,
        0x57f9,0x638a,0x63a5,0x5809,0x63b7,0x63b8,0x63a7,0x6369,0x63ae,0x63a2,0x60ab,0x57ed,0x57fd,0x636e,0x6398,0x63ba,0x6387,0x63bc,0x804c,0x8043,0x57fa,
        0x8046,0x52d8,0x804a,0x804d,0x5a36,0x83c1,0x83dd,0x8457,0x83f1,0x8401,0x83e5,0x83d8,0x5807,0x52d2,0x9ec4,0x8418,0x840b,0x52e9,0x83f2,0x83fd,0x83d6,
        0x840c,0x841c,0x841d,0x83cc,0x840e,0x8438,0x8411,0x83c2,0x83dc,0x68fb,0x83d4,0x83df,0x8404,0x840f,0x83ca,0x8403,0x83e9,0x83fc,0x83cf,0x840d,0x83f9,
        0x83e0,0x83ea,0x83c5,0x83c0,0x8424,0x8425,0x8426,0x4e7e,0x8427,0x83f0,0x83e1,0x8428,0x83c7,0x68b0,0x68bd,0x5f6c,0x68b5,0x68a6,0x5a6a,0x6897,0x68a7,
        0x68be,0x68a2,0x688f,0x6885,0x89cb,0x68c0,0x6874,0x6877,0x6893,0x68b3,0x68c1,0x68af,0x686b,0x68c2,0x6876,0x68ad,0x6551,0x556c,0x90fe,0x532e,0x66f9,
        0x6555,0x526f,0x8c49,0x7968,0x9104,0x915d,0x915e,0x9157,0x915a,0x53a2,0x53a3,0x621a,0x621b,0x784e,0x7845,0x786d,0x7852,0x7855,0x7856,0x7857,0x7850,
        0x785a,0x7847,0x784c,0x9e38,0x74e0,0x530f,0x5962,0x76d4,0x723d,0x53a9,0x804b,0x9f9a,0x88ad,0x6b92,0x6b93,0x6b8d,0x76db,0x8d49,0x533e,0x96e9,0x96ea,
        0x8f84,0x8f85,0x8f86,0x5811,0x9f81,0x9885,0x865a,0x5f6a,0x96c0,0x5802,0x5e38,0x7736,0x772d,0x552a,0x7726,0x5567,0x5319,0x6661,0x6664,0x6668,0x773a,
        0x7735,0x7741,0x772f,0x773c,0x7738,0x60ac,0x91ce,0x570a,0x556a,0x5566,0x558f,0x55b5,0x5549,0x52d6,0x66fc,0x6666,0x665e,0x6657,0x665a,0x5195,0x5544,
        0x556d,0x5561,0x7566,0x8dbc,0x8dba,0x8ddd,0x8dbe,0x5543,0x8dc3,0x556e,0x8dc4,0x7565,0x86b6,0x86c4,0x86ce,0x86c6,0x86b0,0x86ba,0x86ca,0x5709,0x86b1,
        0x86af,0x86c9,0x86c0,0x86c7,0x86cf,0x86b4,0x552c,0x7d2f,0x9102,0x5531,0x60a3,0x5570,0x553e,0x552f,0x5564,0x5565,0x5541,0x5555,0x553f,0x5550,0x553c,
        0x5537,0x5574,0x5556,0x5575,0x5576,0x5577,0x5533,0x5578,0x555c,0x5e3b,0x5d16,0x5d0e,0x5d26,0x5d2d,0x903b,0x5e3c,0x5d2e,0x5d14,0x5e37,0x5d1f,0x5d24,
        0x5d29,0x5d1e,0x5d07,0x5d06,0x5d1b,0x8d47,0x8d48,0x5a74,0x8d4a,0x5708,0x94d0,0x94d1,0x94d2,0x94d5,0x94d7,0x94d8,0x94d9,0x94da,0x94db,0x94dc,0x94dd,
        0x94de,0x94df,0x94e0,0x94e1,0x94e2,0x94e3,0x94e4,0x94e5,0x94e7,0x94e8,0x94e9,0x94ea,0x94eb,0x94ed,0x94ec,0x94ee,0x94ef,0x94f0,0x94f1,0x94f2,0x94f3,
        0x94f4,0x94f5,0x94f6,0x94f7,0x77eb,0x6c2a,0x727e,0x751c,0x9e39,0x79f8,0x68a8,0x7281,0x7a06,0x79fd,0x79fb,0x79fe,0x9036,0x7b3a,0x7b47,0x7b28,0x7b38,
        0x7b3c,0x7b2a,0x7b1b,0x7b19,0x7b2e,0x7b26,0x7b31,0x7b20,0x7b25,0x7b2c,0x7b33,0x7b24,0x7b3e,0x7b1e,0x654f,0x507e,0x505a,0x9e3a,0x5043,0x5055,0x888b,
        0x60a0,0x507f,0x5076,0x5048,0x504e,0x5072,0x5080,0x5077,0x60a8,0x506c,0x552e,0x505c,0x507b,0x504f,0x8eaf,0x7691,0x515c,0x768e,0x5047,0x8845,0x9e3b,
        0x5f98,0x5f99,0x5f9c,0x5f97,0x8854,0x8238,0x823b,0x8233,0x76d8,0x8234,0x8236,0x8239,0x9e3c,0x8237,0x8235,0x659c,0x9f9b,0x76d2,0x9e3d,0x74fb,0x655b,
        0x6089,0x6b32,0x5f69,0x9886,0x7fce,0x811a,0x8116,0x812f,0x8c5a,0x8136,0x8138,0x811e,0x812c,0x8131,0x8118,0x8132,0x8127,0x5310,0x9c7e,0x8c61,0x591f,
        0x9038,0x731c,0x732a,0x730e,0x732b,0x7317,0x51f0,0x7316,0x7321,0x730a,0x731e,0x7304,0x731d,0x659b,0x89d6,0x7315,0x731b,0x9997,0x796d,0x9983,0x9984,
        0x9985,0x9986,0x51d1,0x51cf,0x9e3e,0x6beb,0x5b70,0x70f9,0x5eb6,0x5eb9,0x9ebb,0x5eb5,0x5ebc,0x5ebe,0x5eb3,0x75d4,0x75cd,0x75b5,0x75ca,0x75d2,0x75d5,
        0x5eca,0x5eb7,0x5eb8,0x9e7f,0x76d7,0x7ae0,0x7adf,0x7fca,0x5546,0x65cc,0x65cf,0x65ce,0x65cb,0x671b,0x88a4,0x7387,0x9607,0x9608,0x9609,0x960a,0x960b,
        0x960c,0x960d,0x960e,0x960f,0x9610,0x7740,0x7f9a,0x7f9d,0x7f9f,0x76d6,0x7737,0x7c9d,0x7c98,0x7c97,0x7c95,0x7c92,0x65ad,0x526a,0x517d,0x7110,0x710a,
        0x70ef,0x7113,0x7115,0x70fd,0x7116,0x70f7,0x70fa,0x710c,0x6e05,0x6e0d,0x6dfb,0x6e1a,0x9e3f,0x6dc7,0x6dcb,0x6dc5,0x6dde,0x6e0e,0x6daf,0x6df9,0x6dbf,
        0x6e20,0x6e10,0x6dd1,0x6dd6,0x6332,0x6dcc,0x6dcf,0x6df7,0x6de0,0x6db8,0x6e11,0x6dee,0x6de6,0x6dc6,0x6e0a,0x6deb,0x6ddd,0x6e14,0x6dd8,0x6df3,0x6db2,
        0x6dec,0x6daa,0x6de4,0x6de1,0x6dd9,0x6dc0,0x6dab,0x6df1,0x6e0c,0x6dae,0x6db5,0x5a46,0x6881,0x6e17,0x6dc4,0x60c5,0x60ec,0x60bb,0x60dc,0x60ed,0x60b1,
        0x60bc,0x60dd,0x60e7,0x60d5,0x60d8,0x60b8,0x60df,0x60c6,0x60da,0x60ca,0x60c7,0x60e6,0x60b4,0x60ee,0x60cb,0x60e8,0x60ef,0x5bc7,0x5bc5,0x5bc4,0x5bc2,
        0x902d,0x5bbf,0x7a92,0x7a91,0x7a95,0x5bc6,0x8c0b,0x8c0c,0x8c0d,0x8c0e,0x8c0f,0x6248,0x76b2,0x8c10,0x8c11,0x88c6,0x88b1,0x88bc,0x88c8,0x88c9,0x7977,
        0x7978,0x7972,0x8c12,0x8c13,0x8c14,0x8c15,0x8c16,0x8c17,0x8c19,0x8c1a,0x8c1b,0x8c1c,0x8c1d,0x655d,0x902e,0x902f,0x6562,0x5c09,0x5c60,0x8274,0x5f39,
        0x968b,0x5815,0x90ff,0x968f,0x86cb,0x9685,0x9688,0x7c9c,0x968d,0x9697,0x9686,0x9690,0x5a67,0x5a4a,0x5a5e,0x5a73,0x5a55,0x5a3c,0x5a62,0x5a5a,0x5a75,
        0x5a76,0x5a49,0x80ec,0x8888,0x9887,0x9888,0x7fcc,0x607f,0x6b38,0x7ee9,0x7eea,0x7eeb,0x9a90,0x7eed,0x9a91,0x7eee,0x7eef,0x7ef0,0x9a92,0x7ef2,0x7ef3,
        0x9a93,0x7ef4,0x7ef5,0x7ef6,0x7ef7,0x7ef8,0x7ef9,0x7efa,0x7efb,0x7efc,0x7efd,0x7efe,0x7eff,0x9a96,0x7f00,0x7f01,0x5de2,0x8020,0x742b,0x7435,0x7434,
        0x7436,0x742a,0x745b,0x7433,0x7426,0x7422,0x7425,0x7428,0x9753,0x743c,0x6591,0x7430,0x742e,0x742f,0x742c,0x741b,0x741a,0x8f87,0x66ff,0x9f0b,0x63f3,
        0x63cd,0x6b3e,0x582a,0x581e,0x643d,0x5854,0x642d,0x5843,0x63f8,0x5830,0x63e0,0x5819,0x63e9,0x8d8a,0x8d84,0x8d81,0x8d8b,0x8d85,0x63fd,0x63d0,0x5824,
        0x63d6,0x535a,0x63fe,0x9889,0x63ed,0x559c,0x5f6d,0x63e3,0x5844,0x63ff,0x63d2,0x63ea,0x641c,0x716e,0x5820,0x800b,0x63c4,0x63f4,0x6400,0x86f0,0x86e9,
        0x7d77,0x5846,0x88c1,0x63de,0x6401,0x6413,0x6402,0x6405,0x63ce,0x58f9,0x63e1,0x6452,0x63c6,0x6414,0x63c9,0x63be,0x845c,0x8052,0x65af,0x671f,0x6b3a,
        0x8054,0x8451,0x845a,0x846b,0x9770,0x9778,0x6563,0x8473,0x60f9,0x8487,0x846c,0x8488,0x52df,0x847a,0x845b,0x8489,0x8478,0x843c,0x84c7,0x8429,0x8463,
        0x8446,0x8469,0x8461,0x656c,0x8471,0x848b,0x8476,0x8482,0x848c,0x8453,0x848e,0x843d,0x8431,0x8456,0x97e9,0x621f,0x671d,0x846d,0x8f9c,0x8475,0x68d2,
        0x696e,0x68f1,0x68cb,0x6930,0x690d,0x68ee,0x68fc,0x711a,0x691f,0x6905,0x6912,0x68f9,0x68f5,0x68cd,0x6924,0x68f0,0x690e,0x68c9,0x6911,0x9e40,0x8d4d,
        0x68da,0x690b,0x6901,0x68ec,0x68d5,0x68fa,0x6994,0x6957,0x68e3,0x6910,0x692d,0x9e41,0x60e0,0x60d1,0x903c,0x8983,0x7c9f,0x68d8,0x9163,0x9164,0x9162,
        0x9165,0x9161,0x9166,0x9e42,0x89cc,0x53a8,0x53a6,0x786c,0x785d,0x786a,0x7877,0x786e,0x786b,0x96c1,0x53a5,0x6b96,0x88c2,0x96c4,0x6b9a,0x6b9b,0x988a,
        0x96f3,0x96ef,0x8f8a,0x8f8b,0x6920,0x6682,0x8f8c,0x8f8d,0x8f8e,0x96c5,0x7fd8,0x8f88,0x6590,0x60b2,0x7d2b,0x9ef9,0x8f89,0x655e,0x68e0,0x725a,0x8d4f,
        0x638c,0x6674,0x7750,0x6691,0x6700,0x6670,0x91cf,0x7751,0x7747,0x9f0e,0x7743,0x55b7,0x6222,0x558b,0x55d2,0x5583,0x55b3,0x6676,0x5587,0x9047,0x558a,
        0x55b1,0x55b9,0x904f,0x6677,0x667e,0x666f,0x5588,0x7574,0x8df5,0x8dd6,0x8dcb,0x8dcc,0x8dd7,0x8dde,0x8dda,0x8dd1,0x8dce,0x8dcf,0x8ddb,0x8dc6,0x9057,
        0x86d9,0x86f1,0x86f2,0x86ed,0x86f3,0x86d0,0x86d4,0x86db,0x8713,0x86de,0x8712,0x86e4,0x86f4,0x86df,0x86d8,0x86d1,0x756f,0x5581,0x559d,0x9e43,0x5582,
        0x559f,0x659d,0x5598,0x557e,0x55d6,0x55a4,0x5589,0x55bb,0x5591,0x557c,0x55df,0x55bd,0x55de,0x55a7,0x5580,0x5594,0x5599,0x5d4c,0x5d58,0x5d56,0x5e45,
        0x5d34,0x9044,0x8a48,0x5e3d,0x5d4e,0x5d3d,0x5d5a,0x5d6c,0x5d5b,0x7fd9,0x5d6f,0x5d5d,0x5d6b,0x5e44,0x5d4b,0x8d4b,0x8d4c,0x8d4e,0x8d50,0x8d51,0x8d54,
        0x9ed1,0x94f8,0x94f9,0x94fa,0x94fb,0x94fc,0x94fd,0x94fe,0x94ff,0x9500,0x9501,0x9503,0x9504,0x9502,0x9505,0x9506,0x9507,0x9508,0x9509,0x950a,0x950b,
        0x950c,0x950e,0x950f,0x9510,0x9511,0x9512,0x9513,0x9514,0x9515,0x7525,0x63a3,0x63b0,0x77ed,0x667a,0x77ec,0x6c30,0x6bf3,0x6bef,0x6c2e,0x6bfd,0x6c2f,
        0x728a,0x7284,0x728b,0x9e44,0x728d,0x9e45,0x988b,0x5269,0x5d47,0x7a0d,0x7a0b,0x7a00,0x9ecd,0x7a03,0x7a0e,0x7a02,0x7b50,0x7b49,0x7b58,0x7b51,0x7b56,
        0x7b5a,0x7b5b,0x7b5c,0x7b52,0x7b45,0x7b4f,0x7b75,0x7b4c,0x7b54,0x7b4b,0x7b5d,0x50a3,0x50b2,0x5085,0x5088,0x8204,0x724d,0x724c,0x50a5,0x5821,0x96c6,
        0x7126,0x508d,0x50a7,0x50a8,0x9051,0x7693,0x7696,0x7ca4,0x5965,0x50a9,0x9041,0x8857,0x60e9,0x5fa1,0x5fa8,0x5faa,0x823e,0x8247,0x8212,0x7572,0x5f11,
        0x903e,0x988c,0x7fd5,0x91c9,0x756a,0x91ca,0x9e46,0x79bd,0x821c,0x8c82,0x8148,0x814a,0x814c,0x8153,0x8146,0x8174,0x813e,0x814b,0x8151,0x8159,0x815a,
        0x8154,0x8155,0x8171,0x8152,0x9c7f,0x9c80,0x9c81,0x9c82,0x9c83,0x988d,0x7322,0x7339,0x7329,0x7325,0x732c,0x733e,0x7334,0x98d3,0x89de,0x89da,0x7338,
        0x7331,0x60eb,0x98e7,0x7136,0x9987,0x9988,0x9989,0x998a,0x998b,0x4eb5,0x88c5,0x86ee,0x8114,0x5c31,0x6566,0x88d2,0x5ecb,0x658c,0x75e3,0x75e8,0x75e6,
        0x75d8,0x75de,0x75e2,0x75e4,0x75ea,0x75eb,0x75e7,0x75db,0x910c,0x8d53,0x7ae6,0x7ae5,0x74ff,0x7ae3,0x557b,0x988f,0x9e47,0x9611,0x9612,0x9614,0x9615,
        0x5584,0x7fd4,0x7fa1,0x666e,0x7caa,0x7c9e,0x5c0a,0x5960,0x9052,0x9053,0x9042,0x5b73,0x66fe,0x712f,0x711c,0x7130,0x7119,0x7131,0x9e48,0x6e5b,0x6e2f,
        0x6e2b,0x6ede,0x6e56,0x6e58,0x6e23,0x6e24,0x6e6e,0x6e4e,0x6e5d,0x6e68,0x6e5c,0x6e3a,0x6e7f,0x6e29,0x6e34,0x6e2d,0x6e83,0x6e4d,0x6e85,0x6ed1,0x6e43,
        0x6e6b,0x6eb2,0x6e5f,0x6e86,0x6e1d,0x6e72,0x6e7e,0x6e21,0x6e38,0x6ea0,0x6e87,0x6e54,0x6ecb,0x6e49,0x6e32,0x6e89,0x6e25,0x6e44,0x6ec1,0x6124,0x614c,
        0x60f0,0x6120,0x60fa,0x6126,0x6115,0x60f4,0x6123,0x6100,0x610e,0x60f6,0x6127,0x6109,0x6114,0x6168,0x55be,0x5272,0x5bd2,0x5bcc,0x5bd3,0x7a9c,0x7a9d,
        0x7a96,0x7a97,0x7a98,0x5bd0,0x8c1f,0x6249,0x904d,0x68e8,0x96c7,0x624a,0x88e2,0x88ce,0x88e3,0x88d5,0x88e4,0x88e5,0x88d9,0x797e,0x797a,0x797c,0x8c20,
        0x7985,0x7984,0x5e42,0x8c21,0x8c22,0x8c23,0x8c24,0x8c25,0x8c26,0x8c27,0x5848,0x9050,0x7280,0x5c5e,0x5c61,0x5b71,0x5f3c,0x5f3a,0x7ca5,0x5dfd,0x758f,
        0x9694,0x9a98,0x9699,0x9698,0x5a92,0x5aaa,0x7d6e,0x5ac2,0x5a9b,0x5a77,0x5a9a,0x5a7f,0x5def,0x6bf5,0x7fda,0x767b,0x76b4,0x5a7a,0x9a9b,0x7f02,0x7f03,
        0x7f04,0x7f05,0x5f58,0x7f06,0x7f07,0x7f08,0x7f09,0x7f0c,0x7f0e,0x7f0f,0x7f11,0x7f12,0x7f13,0x7f14,0x7f15,0x9a97,0x7f16,0x7f17,0x9a99,0x9a9a,0x7f18,
        0x98e8,0x8022,0x745f,0x745a,0x9e49,0x7441,0x745e,0x7470,0x7440,0x745c,0x7457,0x7444,0x7455,0x9068,0x9a9c,0x7459,0x9058,0x97eb,0x9b42,0x9ae1,0x8086,
        0x6444,0x6478,0x586b,0x640f,0x5865,0x586c,0x9122,0x8d94,0x8d91,0x6445,0x584c,0x6441,0x9f13,0x6446,0x8d6a,0x643a,0x586e,0x8707,0x640b,0x642c,0x6447,
        0x641e,0x642a,0x5858,0x6412,0x6410,0x641b,0x6420,0x6448,0x5f40,0x6bc2,0x640c,0x6426,0x644a,0x6421,0x8058,0x84c1,0x6221,0x659f,0x849c,0x84cd,0x911e,
        0x52e4,0x9774,0x9773,0x9776,0x9e4a,0x84d0,0x84dd,0x5893,0x5e55,0x84e6,0x9e4b,0x84bd,0x84d3,0x84d6,0x84ca,0x84af,0x84df,0x84ec,0x84d1,0x84bf,0x84ba,
        0x84e0,0x849f,0x84a1,0x84c4,0x84b9,0x84b4,0x84b2,0x8497,0x84c9,0x8499,0x84c2,0x84e5,0x9890,0x84b8,0x732e,0x84e3,0x6954,0x693f,0x6960,0x7981,0x6942,
        0x695a,0x695d,0x6977,0x6984,0x60f3,0x696b,0x6980,0x695e,0x6978,0x6934,0x69d0,0x69cc,0x696f,0x6986,0x6987,0x6988,0x69ce,0x697c,0x6989,0x6966,0x6982,
        0x6963,0x6979,0x693d,0x88d8,0x8d56,0x527d,0x7504,0x916e,0x9170,0x916f,0x916a,0x9169,0x916c,0x8703,0x611f,0x789b,0x788d,0x7898,0x7893,0x7891,0x787c,
        0x7889,0x788e,0x789a,0x78b0,0x7887,0x7897,0x788c,0x789c,0x9e4c,0x5c34,0x96f7,0x96f6,0x96fe,0x96f9,0x8f8f,0x8f90,0x8f91,0x8f92,0x8f93,0x7763,0x9891,
        0x9f83,0x9f84,0x9f85,0x9f86,0x89dc,0x8a3e,0x7cb2,0x865e,0x9274,0x775b,0x7779,0x7766,0x7784,0x775a,0x55ea,0x776b,0x97ea,0x55f7,0x55c9,0x7761,0x7768,
        0x7762,0x96ce,0x7765,0x776c,0x561f,0x55dc,0x55d1,0x55eb,0x55ec,0x55d4,0x9119,0x55e6,0x55dd,0x611a,0x6225,0x55c4,0x6696,0x76df,0x7166,0x6b47,0x6697,
        0x6685,0x6684,0x6687,0x7167,0x9062,0x668c,0x7578,0x8dec,0x8de8,0x8df6,0x8df7,0x8df8,0x8dd0,0x8de3,0x8df9,0x8df3,0x8dfa,0x8dea,0x8def,0x8dfb,0x8de4,
        0x8ddf,0x9063,0x86f8,0x8708,0x870e,0x8717,0x86fe,0x870a,0x870d,0x8709,0x8702,0x8723,0x8715,0x7579,0x86f9,0x55e3,0x55ef,0x55c5,0x55e5,0x55f2,0x55f3,
        0x55e1,0x55cc,0x55cd,0x55e8,0x55e4,0x55f5,0x55d3,0x7f72,0x7f6e,0x7f68,0x7f6a,0x7f69,0x8700,0x5e4c,0x5d4a,0x5d69,0x5d74,0x9ab0,0x9516,0x9517,0x9519,
        0x9518,0x951a,0x951b,0x951c,0x951d,0x951e,0x951f,0x9521,0x9522,0x9523,0x9524,0x9525,0x9526,0x9527,0x9528,0x952a,0x952b,0x9529,0x952c,0x952d,0x952e,
        0x952f,0x9530,0x9531,0x77ee,0x96c9,0x6c32,0x728f,0x8f9e,0x6b43,0x7a1e,0x7a1a,0x7a17,0x7a14,0x7a20,0x9893,0x6101,0x7b79,0x7b60,0x7b62,0x7b6e,0x7b7b,
        0x7b72,0x7b7c,0x7b71,0x7b7e,0x7b80,0x7b77,0x6bc1,0x8205,0x9f20,0x7252,0x7172,0x50ac,0x50bb,0x50cf,0x8eb2,0x9e4e,0x9b41,0x656b,0x50c7,0x8859,0x5fae,
        0x5fad,0x6106,0x8244,0x89ce,0x6bf9,0x6108,0x9065,0x8c8a,0x8c85,0x8c89,0x9894,0x817b,0x8160,0x8169,0x8170,0x817c,0x817d,0x8165,0x816e,0x816d,0x8179,
        0x817a,0x8167,0x9e4f,0x584d,0x5ab5,0x817e,0x817f,0x8a79,0x9c85,0x9c86,0x9c87,0x9c88,0x9c89,0x9c8a,0x7a23,0x9c8b,0x9c8c,0x9c8d,0x9c8f,0x9c90,0x8084,
        0x733f,0x9896,0x9e50,0x98d4,0x98d5,0x89e5,0x89e6,0x89e3,0x905b,0x715e,0x96cf,0x998c,0x998d,0x998f,0x9990,0x9171,0x9e51,0x7980,0x4eb6,0x5ed2,0x7603,
        0x75f1,0x75f9,0x75fc,0x5ed3,0x75f4,0x75ff,0x7610,0x7601,0x7605,0x75f0,0x7606,0x5ec9,0x9118,0x9e82,0x88d4,0x9756,0x65b0,0x9123,0x6b46,0x97f5,0x610f,
        0x65d2,0x96cd,0x9616,0x9617,0x9618,0x9619,0x7fa7,0x8c62,0x8a8a,0x7cb3,0x7cae,0x6570,0x714e,0x7337,0x5851,0x6148,0x7164,0x7173,0x715c,0x7168,0x7145,
        0x714c,0x714a,0x7178,0x717a,0x6edf,0x6eb1,0x6e98,0x6ee0,0x6ee1,0x6f2d,0x6f20,0x6ee2,0x6ec7,0x6ea5,0x6ea7,0x6ebd,0x6e90,0x6ee4,0x6ee5,0x88df,0x6ebb,
        0x6eb7,0x6ea6,0x6ed7,0x6eeb,0x6eb4,0x6ecf,0x6ed4,0x6eaa,0x6ec3,0x6e9c,0x6ee6,0x6f13,0x6eda,0x6e8f,0x6ec2,0x6ea2,0x6eaf,0x6ee8,0x6eb6,0x6ed3,0x6e9f,
        0x6ed8,0x6eba,0x6ecd,0x7cb1,0x6ee9,0x6eea,0x612b,0x6151,0x614e,0x6165,0x614a,0x8a89,0x9c8e,0x585e,0x9a9e,0x5bde,0x7aa5,0x7aa6,0x7aa0,0x7aa3,0x7a9f,
        0x5bdd,0x8c28,0x88f1,0x8902,0x891a,0x88f8,0x88fc,0x88e8,0x88fe,0x88f0,0x798a,0x798f,0x8c29,0x8c2a,0x8c2b,0x8c2c,0x7fa4,0x6bbf,0x8f9f,0x969c,0x5abe,
        0x5aeb,0x5ab3,0x5ab2,0x5ad2,0x5ac9,0x5acc,0x5ac1,0x5ad4,0x5ab8,0x53e0,0x7f19,0x7f1c,0x7f1a,0x7f1b,0x8f94,0x7f1d,0x9a9d,0x7f1f,0x7f20,0x7f21,0x7f22,
        0x7f23,0x7f24,0x9a9f,0x527f,0x8025,0x7488,0x9759,0x78a7,0x7476,0x7483,0x746d,0x7462,0x7352,0x8d58,0x71ac,0x89cf,0x615d,0x5ae0,0x97ec,0x9ae6,0x5888,
        0x5899,0x647d,0x589f,0x6487,0x5881,0x6482,0x645e,0x5609,0x6467,0x6484,0x8d6b,0x622a,0x7fe5,0x8e05,0x8a93,0x928e,0x646d,0x5889,0x5883,0x6458,0x5892,
        0x6454,0x6996,0x6496,0x647a,0x7da6,0x805a,0x852b,0x8537,0x977a,0x977c,0x9785,0x977d,0x9781,0x977f,0x850c,0x853d,0x6155,0x66ae,0x6479,0x8513,0x8511,
        0x750d,0x8538,0x84f0,0x8539,0x8521,0x8517,0x851f,0x853a,0x622c,0x853d,0x8556,0x853b,0x84ff,0x853c,0x65a1,0x7199,0x851a,0x9e55,0x5162,0x560f,0x84fc,
        0x699b,0x69a7,0x6a21,0x69da,0x69db,0x69bb,0x69ab,0x69dc,0x69ad,0x69d4,0x69b4,0x69c1,0x699c,0x69df,0x69a8,0x6995,0x69e0,0x69b7,0x698d,0x6b4c,0x906d,
        0x50f0,0x9175,0x917d,0x917e,0x9172,0x9177,0x9176,0x9174,0x9179,0x917f,0x9178,0x53ae,0x78b6,0x78a1,0x789f,0x78b4,0x78b1,0x78a3,0x78b3,0x78b2,0x78cb,
        0x78c1,0x78b9,0x78a5,0x613f,0x5282,0x81e7,0x8c68,0x6ba1,0x9700,0x9706,0x9701,0x8f95,0x8f96,0x8f97,0x871a,0x88f4,0x7fe1,0x96cc,0x9f87,0x9f88,0x777f,
        0x5f0a,0x88f3,0x9897,0x5925,0x7785,0x778d,0x777d,0x5885,0x561e,0x5608,0x55fd,0x560c,0x5601,0x560e,0x66a7,0x669d,0x8e0c,0x8e09,0x8dfd,0x8e0a,0x873b,
        0x871e,0x8721,0x8725,0x872e,0x873e,0x8748,0x8734,0x8747,0x8718,0x8731,0x8729,0x8737,0x8749,0x873f,0x8782,0x8722,0x5618,0x5621,0x9e57,0x5623,0x5624,
        0x561a,0x561b,0x5600,0x55fe,0x5627,0x7f74,0x7f71,0x5e54,0x5d82,0x5e5b,0x8d59,0x7f42,0x8d5a,0x9ab7,0x9ab6,0x9e58,0x9532,0x9534,0x9536,0x9537,0x9538,
        0x9539,0x953b,0x953d,0x953e,0x9535,0x953f,0x9540,0x9541,0x9542,0x9543,0x9544,0x9545,0x821e,0x7292,0x8214,0x7a33,0x718f,0x7b90,0x7ba6,0x7ba7,0x7b8d,
        0x7bb8,0x7ba8,0x7b95,0x7bac,0x7b97,0x7b85,0x7ba9,0x7baa,0x7b94,0x7ba1,0x7b9c,0x7ba2,0x7bab,0x7b93,0x6bd3,0x8206,0x50d6,0x5106,0x50f3,0x50da,0x50ed,
        0x50ec,0x5281,0x50e6,0x50ee,0x50e7,0x9f3b,0x9b44,0x9b45,0x9b43,0x9b46,0x777e,0x824b,0x9131,0x8c8c,0x819c,0x818a,0x8188,0x8180,0x8191,0x9c91,0x9c94,
        0x9c99,0x9c9a,0x9c9b,0x9c9c,0x9c9f,0x7591,0x7350,0x734d,0x98d7,0x89eb,0x96d2,0x5b75,0x5924,0x9991,0x9992,0x92ae,0x88f9,0x6572,0x8c6a,0x818f,0x587e,
        0x906e,0x9ebd,0x5ed9,0x8150,0x7629,0x760c,0x7617,0x761f,0x7626,0x760a,0x7625,0x7618,0x7619,0x5ed6,0x8fa3,0x5f70,0x7aed,0x97f6,0x7aef,0x65d7,0x65d6,
        0x8182,0x961a,0x912f,0x9c9e,0x7cbe,0x7cbc,0x7cb9,0x7cbd,0x7cc1,0x6b49,0x69ca,0x9e5a,0x5f0a,0x7184,0x7198,0x7194,0x717d,0x71a5,0x6f62,0x6f46,0x6f47,
        0x6f24,0x6f06,0x6f15,0x6f31,0x6f02,0x6ef9,0x6f2b,0x6f2f,0x6f36,0x6f4b,0x6f74,0x6f2a,0x6f09,0x6f33,0x6ef4,0x6f29,0x6f3e,0x6f14,0x6f89,0x6f0f,0x6f4d,
        0x6162,0x6177,0x6175,0x5be8,0x8d5b,0x6434,0x5be1,0x7aac,0x7aa8,0x7aad,0x5bdf,0x871c,0x5be4,0x5be5,0x8c2d,0x8087,0x7dae,0x8c2e,0x8921,0x8919,0x8910,
        0x8913,0x891b,0x890a,0x892a,0x799a,0x8c2f,0x8c30,0x8c31,0x8c32,0x66a8,0x5c63,0x9e5b,0x96a7,0x5ae3,0x5af1,0x5ae9,0x5ad6,0x5ae6,0x5ada,0x5ad8,0x5adc,
        0x5ae1,0x5aea,0x9f10,0x7fdf,0x7fe0,0x718a,0x51f3,0x7780,0x9e5c,0x9aa0,0x7f25,0x7f26,0x7f27,0x9aa1,0x7f28,0x9aa2,0x7f29,0x7f2a,0x7f2b,0x6167,0x8026,
        0x8027,0x747e,0x749c,0x7480,0x748e,0x7481,0x748b,0x7487,0x7486,0x596d,0x64b5,0x9aef,0x9aeb,0x64b7,0x6495,0x6492,0x6485,0x64a9,0x8da3,0x8d9f,0x6491,
        0x64ae,0x64ac,0x8d6d,0x64ad,0x58a6,0x64d2,0x64b8,0x92c6,0x58a9,0x649e,0x64a4,0x6499,0x589e,0x64ba,0x5880,0x64b0,0x8069,0x806a,0x89d0,0x978b,0x9791,
        0x8559,0x9792,0x978d,0x8548,0x8568,0x8564,0x855e,0x857a,0x77a2,0x8549,0x5290,0x8543,0x8572,0x8570,0x854a,0x8d5c,0x852c,0x8574,0x9f12,0x69ff,0x6a2a,
        0x6a2f,0x69fd,0x69ed,0x6a17,0x6a18,0x6a31,0x6a0a,0x6a61,0x69f2,0x6a1f,0x6a44,0x6577,0x9e5d,0x8c4c,0x98d8,0x918b,0x918c,0x9187,0x9189,0x9185,0x9765,
        0x9b47,0x990d,0x78d5,0x78ca,0x78d4,0x78d9,0x78c5,0x78be,0x78c9,0x6ba3,0x616d,0x9707,0x9704,0x9709,0x9708,0x8f98,0x9f89,0x9f8a,0x89d1,0x618b,0x778c,
        0x7792,0x9898,0x66b4,0x778e,0x7791,0x563b,0x562d,0x564e,0x5636,0x5676,0x5632,0x9899,0x66b9,0x5639,0x5f71,0x8e14,0x8e1d,0x8e22,0x8e0f,0x8e1f,0x8e2c,
        0x8e29,0x8e2e,0x8e23,0x8e2f,0x8e2a,0x8e3a,0x8e1e,0x877d,0x8776,0x877e,0x8774,0x877b,0x8760,0x8770,0x874e,0x874c,0x876e,0x878b,0x8757,0x8753,0x8763,
        0x877c,0x8764,0x8759,0x5657,0x562c,0x989a,0x563f,0x564d,0x5662,0x5659,0x565c,0x564c,0x5631,0x5640,0x5654,0x989b,0x5e5e,0x5e61,0x5d93,0x5e62,0x5d99,
        0x5d9d,0x58a8,0x9aba,0x9abc,0x9ab8,0x954a,0x9546,0x9547,0x9548,0x9549,0x954b,0x954c,0x954d,0x954e,0x954f,0x9550,0x9551,0x9552,0x9553,0x9554,0x9760,
        0x7a3d,0x7a37,0x7a3b,0x9ece,0x7a3f,0x7a3c,0x7bb1,0x7bb4,0x7bd1,0x7bc1,0x7bcc,0x7bd3,0x7bad,0x7bc7,0x7bc6,0x50f5,0x7256,0x5107,0x510b,0x8eba,0x50fb,
        0x5fb7,0x5fb5,0x8258,0x78d0,0x8662,0x9e5e,0x9e5f,0x819d,0x8198,0x819b,0x6ed5,0x9ca0,0x9ca1,0x9ca2,0x9ca3,0x9ca5,0x9ca4,0x9ca6,0x9ca7,0x9ca9,0x9caa,
        0x9cab,0x9cac,0x6a65,0x7357,0x7360,0x89ef,0x9e60,0x9993,0x9994,0x719f,0x6469,0x9ebe,0x8912,0x5edb,0x9e61,0x761b,0x763c,0x762a,0x7622,0x7624,0x7620,
        0x762b,0x9f51,0x9e61,0x51db,0x989c,0x6bc5,0x7faf,0x7fb0,0x7cca,0x7cc7,0x9074,0x7ccc,0x7ccd,0x7cc8,0x7cc5,0x7fe6,0x9075,0x9e63,0x618b,0x719c,0x71b5,
        0x71a0,0x6f5c,0x6f8d,0x6f8e,0x6f8c,0x6f75,0x6f6e,0x6f78,0x6f6d,0x6f66,0x9ca8,0x6f72,0x92c8,0x6f5f,0x6fb3,0x6f58,0x6f7c,0x6f88,0x6f9c,0x6f7d,0x6f7a,
        0x6f84,0x6f4f,0x61c2,0x61ac,0x6194,0x61ca,0x61a7,0x618e,0x5bee,0x7ab3,0x989d,0x8c33,0x7fe9,0x8925,0x8934,0x892b,0x79a4,0x8c34,0x9e64,0x8c35,0x61a8,
        0x71a8,0x6170,0x5288,0x5c65,0x5c66,0x5b09,0x52f0,0x622e,0x8765,0x8c6b,0x7f2c,0x7f2d,0x7f2e,0x7f2f,0x9aa3,0x757f,0x8029,0x8028,0x802a,0x749e,0x749f,
        0x975b,0x74a0,0x7498,0x8071,0x87af,0x9afb,0x9aed,0x9af9,0x64c0,0x64bc,0x64c2,0x64cd,0x71b9,0x750f,0x64d0,0x64c5,0x64de,0x78ec,0x9139,0x989e,0x857b,
        0x9798,0x71d5,0x9ec7,0x989f,0x85a4,0x857e,0x85af,0x85a8,0x859b,0x8587,0x6aa0,0x64ce,0x85aa,0x858f,0x8579,0x85ae,0x8584,0x98a0,0x7ff0,0x5669,0x859c,
        0x8585,0x6a3e,0x6a71,0x6a5b,0x6a47,0x6a35,0x6a8e,0x6a79,0x6a66,0x6a3d,0x6a28,0x6a59,0x6a58,0x6a7c,0x58bc,0x6574,0x6a50,0x878d,0x7fee,0x74e2,0x919b,
        0x9190,0x918d,0x9192,0x919a,0x9191,0x89f1,0x78fa,0x78f2,0x8d5d,0x98d9,0x6baa,0x9716,0x970f,0x9713,0x970d,0x970e,0x933e,0x8f99,0x8f9a,0x81fb,0x5180,
        0x9910,0x907d,0x6c05,0x77a5,0x779f,0x77a0,0x77b0,0x5684,0x5686,0x5664,0x66be,0x66c8,0x8e40,0x8e45,0x8e36,0x8e39,0x8e35,0x8e3d,0x5634,0x8e31,0x8e44,
        0x8e49,0x8e41,0x8e42,0x87a8,0x87d2,0x87c6,0x8788,0x8785,0x87ad,0x8797,0x8783,0x87a0,0x879f,0x5671,0x5668,0x566a,0x566c,0x566b,0x567b,0x567c,0x5e6a,
        0x7f79,0x571c,0x9e66,0x8d60,0x9ed8,0x9ed4,0x9556,0x9557,0x9558,0x955a,0x955b,0x955c,0x955d,0x955e,0x9560,0x6c07,0x6c06,0x8d5e,0x61a9,0x7a51,0x7a46,
        0x7a44,0x7bdd,0x7bda,0x7be5,0x7bee,0x7be1,0x7c09,0x7be6,0x7bea,0x7bf7,0x7bd9,0x7bf1,0x76e5,0x5112,0x5293,0x7ff1,0x9b49,0x9b48,0x9080,0x5fbc,0x8861,
        0x6b59,0x76e6,0x81a8,0x81aa,0x81b3,0x87a3,0x81a6,0x8199,0x96d5,0x9cad,0x9cae,0x9caf,0x9cb0,0x9cb1,0x9cb2,0x9cb3,0x9cb4,0x9cb5,0x9cb7,0x9cb8,0x9cba,
        0x9cb9,0x9cbb,0x7374,0x736d,0x736c,0x9082,0x619d,0x4eb8,0x9e67,0x78e8,0x5ee8,0x8d5f,0x7640,0x762d,0x7630,0x5eea,0x763f,0x7635,0x7634,0x7643,0x763e,
        0x7638,0x7633,0x6593,0x9e87,0x9e88,0x51dd,0x8fa8,0x8fa9,0x5b34,0x58c5,0x7fb2,0x7cd9,0x7cd7,0x7cd6,0x7cd5,0x77a5,0x7511,0x71ce,0x71e0,0x71d4,0x71c3,
        0x71e7,0x71ca,0x71cf,0x6fd1,0x6fd2,0x6fc9,0x6f5e,0x6fa7,0x6fa1,0x6fb4,0x6fc0,0x6fb9,0x6fa5,0x6fb6,0x6fc2,0x6fbc,0x61b7,0x61d2,0x61be,0x61c8,0x9ec9,
        0x8930,0x5bf0,0x7ab8,0x7abf,0x8936,0x79a7,0x58c1,0x907f,0x5b16,0x729f,0x96b0,0x5b17,0x9e68,0x7fef,0x98a1,0x7f30,0x7f31,0x7f32,0x7f33,0x7f34,0x74a8,
        0x74a9,0x7490,0x74aa,0x6234,0x87ab,0x64e4,0x58d5,0x64e6,0x89f3,0x7f44,0x64e2,0x85c9,0x85b9,0x97a1,0x97a0,0x85cf,0x85b7,0x85b0,0x85d0,0x85d3,0x85c1,
        0x6aac,0x6a91,0x6a84,0x6a90,0x6aa9,0x6a80,0x61cb,0x91a2,0x7ff3,0x7e44,0x7901,0x7905,0x78f7,0x78f4,0x9e69,0x971c,0x971e,0x9f8b,0x9f8c,0x8c73,0x58d1,
        0x9efb,0x77ad,0x77a7,0x77ac,0x77b3,0x77b5,0x77a9,0x77aa,0x568f,0x66d9,0x5685,0x8e51,0x8e52,0x8e4b,0x8e48,0x8e4a,0x8e53,0x8e50,0x87e5,0x87ac,0x87b5,
        0x7583,0x87b3,0x87ba,0x87cb,0x87d1,0x87c0,0x568e,0x5693,0x7f81,0x7f7d,0x7f7e,0x5db7,0x8d61,0x9edc,0x9edd,0x9ac1,0x9ac0,0x9561,0x9562,0x9563,0x9564,
        0x9565,0x9566,0x9567,0x9568,0x9569,0x956a,0x956b,0x7f45,0x7a57,0x9ecf,0x9b4f,0x7c27,0x7c0c,0x7bfe,0x7c03,0x7bfc,0x7c0f,0x7c07,0x7c16,0x7c0b,0x7e41,
        0x9f22,0x9edb,0x5121,0x9e6a,0x9f3e,0x76a4,0x9b4d,0x5fbd,0x825a,0x9fa0,0x7235,0x7e47,0x8c98,0x9088,0x8c94,0x81cc,0x6726,0x81ca,0x81bb,0x81c1,0x81c6,
        0x81c3,0x9cbc,0x9cbd,0x9cbe,0x9cc0,0x9cc1,0x9cc2,0x9cc3,0x9cc4,0x9cc5,0x9cc6,0x9cc7,0x9cc8,0x9cc9,0x9cca,0x736f,0x87bd,0x71ee,0x9e6b,0x8944,0x7cdc,
        0x7e3b,0x81ba,0x764d,0x764c,0x9e8b,0x8fab,0x8d62,0x7cdf,0x7ce0,0x9998,0x71e5,0x61d1,0x6fe1,0x6fee,0x6fde,0x6fe0,0x6fef,0x61e6,0x8c41,0x8e47,0x8b07,
        0x9083,0x8955,0x8941,0x81c0,0x6a97,0x7513,0x81c2,0x64d8,0x5b7a,0x96b3,0x5b37,0x7ffc,0x87ca,0x9e6c,0x936a,0x9aa4,0x93ca,0x9ccc,0x9b39,0x9b08,0x9b03,
        0x77bd,0x85d5,0x97af,0x97a8,0x97ad,0x97ab,0x97a7,0x97a3,0x85dc,0x85e0,0x85e4,0x85e9,0x9e72,0x6aab,0x6ab5,0x8986,0x91aa,0x8e59,0x791e,0x7913,0x790c,
        0x71f9,0x992e,0x8e69,0x77bf,0x77bb,0x66db,0x98a2,0x66dc,0x8e87,0x8e66,0x9e6d,0x8e62,0x8e5c,0x87db,0x87ea,0x87e0,0x87ee,0x569a,0x56a3,0x9e6e,0x9ee0,
        0x9edf,0x9ac5,0x9ac2,0x956c,0x956d,0x956f,0x9570,0x9571,0x99a5,0x7c20,0x7c1f,0x7c2a,0x7c26,0x9f2b,0x9f2c,0x9f29,0x96e0,0x825f,0x7ffb,0x81d1,0x9ccd,
        0x9cce,0x9ccf,0x9cd0,0x9cd0,0x9cd1,0x9e71,0x9e70,0x765e,0x7654,0x765c,0x7656,0x7ce8,0x5181,0x7011,0x700d,0x700c,0x938f,0x61f5,0x895f,0x74a7,0x6233,
        0x5f5d,0x908b,0x9b0f,0x6509,0x6512,0x97b2,0x97b4,0x85ff,0x8627,0x5b7d,0x8605,0x8b66,0x8611,0x85fb,0x9e93,0x6500,0x91ad,0x91ae,0x91af,0x7924,0x9143,
        0x972a,0x972d,0x9efc,0x9cd6,0x66dd,0x56af,0x8e70,0x8e76,0x8e7d,0x8e7c,0x8e6f,0x8e74,0x8e7e,0x8e72,0x8e6d,0x8e7f,0x8e6c,0x8816,0x8813,0x880b,0x87fe,
        0x880a,0x5dc5,0x9ee2,0x9acb,0x9acc,0x9572,0x7c40,0x7c38,0x7c41,0x7c3f,0x9cd8,0x9f41,0x9b51,0x8268,0x9f17,0x9cd3,0x9cd4,0x9cd5,0x9cd7,0x9cd9,0x9cda,
        0x87f9,0x98a4,0x9761,0x7663,0x9e92,0x93d6,0x74e3,0x8803,0x7fb8,0x7fb9,0x7206,0x701a,0x7023,0x701b,0x8966,0x8c36,0x895e,0x7586,0x9aa5,0x7f35,0x74d2,
        0x9b13,0x58e4,0x6518,0x99a8,0x8629,0x8616,0x8618,0x91b5,0x91b4,0x9730,0x98a5,0x9146,0x8000,0x77cd,0x66e6,0x8e81,0x8e85,0x8815,0x9f0d,0x56bc,0x56b7,
        0x5dcd,0x5dc9,0x9ee9,0x9ee5,0x9573,0x9574,0x9ee7,0x7c4d,0x7e82,0x9f2f,0x72a8,0x81dc,0x9cdc,0x9cdd,0x9cde,0x9cdf,0x737e,0x9b54,0x7cef,0x704c,0x7039,
        0x7035,0x8b6c,0x5b40,0x9aa7,0x8030,0x8822,0x74d8,0x9f19,0x91ba,0x7934,0x7933,0x9738,0x9732,0x9739,0x98a6,0x66e9,0x8e8f,0x9eef,0x9ad3,0x9f31,0x9ce1,
        0x9ce2,0x766b,0x9e9d,0x8d63,0x5914,0x721d,0x704f,0x79b3,0x943e,0x7fbc,0x8821,0x8032,0x8031,0x61ff,0x97c2,0x8638,0x9e73,0x7cf5,0x863c,0x56ca,0x973e,
        0x6c0d,0x9955,0x8e94,0x8e90,0x9ad1,0x9575,0x9576,0x7a70,0x9ce4,0x74e4,0x9954,0x9b3b,0x9b1f,0x8db1,0x652b,0x6525,0x98a7,0x8e9c,0x7f50,0x9f39,0x9f37,
        0x766f,0x9e9f,0x8832,0x77d7,0x8839,0x91be,0x8e9e,0x8862,0x946b,0x705e,0x897b,0x7e9b,0x9b23,0x652e,0x56d4,0x9995,0x6206,0x883c,0x7228,0x9f49 };

    std::sort(ChinenseCommonMat, ChinenseCommonMat + 7007);
    static short accumulative_offsets_from_0x4E00[7007];
    accumulative_offsets_from_0x4E00[0] = 0;
    for (int i = 1; i < IM_ARRAYSIZE(ChinenseCommonMat); ++i)
    {
        accumulative_offsets_from_0x4E00[i] = ChinenseCommonMat[i] - ChinenseCommonMat[i - 1];
    }
   
    static ImWchar base_ranges[] = // not zero-terminated
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2000, 0x206F, // General Punctuation
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF  // Half-width characters
    };
    static ImWchar full_ranges[IM_ARRAYSIZE(base_ranges) + IM_ARRAYSIZE(accumulative_offsets_from_0x4E00) * 2 + 1] = { 0 };
    if (!full_ranges[0])
    {
        memcpy(full_ranges, base_ranges, sizeof(base_ranges));
        UnpackAccumulativeOffsetsIntoRanges(0x4E00, accumulative_offsets_from_0x4E00, IM_ARRAYSIZE(accumulative_offsets_from_0x4E00), full_ranges + IM_ARRAYSIZE(base_ranges));
    }
    return &full_ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesJapanese()
{
    // 1946 common ideograms code points for Japanese
    // Sourced from http://theinstructionlimit.com/common-kanji-character-ranges-for-xna-spritefont-rendering
    // FIXME: Source a list of the revised 2136 Joyo Kanji list from 2010 and rebuild this.
    // You can use ImFontGlyphRangesBuilder to create your own ranges derived from this, by merging existing ranges or adding new characters.
    // (Stored as accumulative offsets from the initial unicode codepoint 0x4E00. This encoding is designed to helps us compact the source code size.)
    static const short accumulative_offsets_from_0x4E00[] =
    {
        0,1,2,4,1,1,1,1,2,1,6,2,2,1,8,5,7,11,1,2,10,10,8,2,4,20,2,11,8,2,1,2,1,6,2,1,7,5,3,7,1,1,13,7,9,1,4,6,1,2,1,10,1,1,9,2,2,4,5,6,14,1,1,9,3,18,
        5,4,2,2,10,7,1,1,1,3,2,4,3,23,2,10,12,2,14,2,4,13,1,6,10,3,1,7,13,6,4,13,5,2,3,17,2,2,5,7,6,4,1,7,14,16,6,13,9,15,1,1,7,16,4,7,1,19,9,2,7,15,
        2,6,5,13,25,4,14,13,11,25,1,1,1,2,1,2,2,3,10,11,3,3,1,1,4,4,2,1,4,9,1,4,3,5,5,2,7,12,11,15,7,16,4,5,16,2,1,1,6,3,3,1,1,2,7,6,6,7,1,4,7,6,1,1,
        2,1,12,3,3,9,5,8,1,11,1,2,3,18,20,4,1,3,6,1,7,3,5,5,7,2,2,12,3,1,4,2,3,2,3,11,8,7,4,17,1,9,25,1,1,4,2,2,4,1,2,7,1,1,1,3,1,2,6,16,1,2,1,1,3,12,
        20,2,5,20,8,7,6,2,1,1,1,1,6,2,1,2,10,1,1,6,1,3,1,2,1,4,1,12,4,1,3,1,1,1,1,1,10,4,7,5,13,1,15,1,1,30,11,9,1,15,38,14,1,32,17,20,1,9,31,2,21,9,
        4,49,22,2,1,13,1,11,45,35,43,55,12,19,83,1,3,2,3,13,2,1,7,3,18,3,13,8,1,8,18,5,3,7,25,24,9,24,40,3,17,24,2,1,6,2,3,16,15,6,7,3,12,1,9,7,3,3,
        3,15,21,5,16,4,5,12,11,11,3,6,3,2,31,3,2,1,1,23,6,6,1,4,2,6,5,2,1,1,3,3,22,2,6,2,3,17,3,2,4,5,1,9,5,1,1,6,15,12,3,17,2,14,2,8,1,23,16,4,2,23,
        8,15,23,20,12,25,19,47,11,21,65,46,4,3,1,5,6,1,2,5,26,2,1,1,3,11,1,1,1,2,1,2,3,1,1,10,2,3,1,1,1,3,6,3,2,2,6,6,9,2,2,2,6,2,5,10,2,4,1,2,1,2,2,
        3,1,1,3,1,2,9,23,9,2,1,1,1,1,5,3,2,1,10,9,6,1,10,2,31,25,3,7,5,40,1,15,6,17,7,27,180,1,3,2,2,1,1,1,6,3,10,7,1,3,6,17,8,6,2,2,1,3,5,5,8,16,14,
        15,1,1,4,1,2,1,1,1,3,2,7,5,6,2,5,10,1,4,2,9,1,1,11,6,1,44,1,3,7,9,5,1,3,1,1,10,7,1,10,4,2,7,21,15,7,2,5,1,8,3,4,1,3,1,6,1,4,2,1,4,10,8,1,4,5,
        1,5,10,2,7,1,10,1,1,3,4,11,10,29,4,7,3,5,2,3,33,5,2,19,3,1,4,2,6,31,11,1,3,3,3,1,8,10,9,12,11,12,8,3,14,8,6,11,1,4,41,3,1,2,7,13,1,5,6,2,6,12,
        12,22,5,9,4,8,9,9,34,6,24,1,1,20,9,9,3,4,1,7,2,2,2,6,2,28,5,3,6,1,4,6,7,4,2,1,4,2,13,6,4,4,3,1,8,8,3,2,1,5,1,2,2,3,1,11,11,7,3,6,10,8,6,16,16,
        22,7,12,6,21,5,4,6,6,3,6,1,3,2,1,2,8,29,1,10,1,6,13,6,6,19,31,1,13,4,4,22,17,26,33,10,4,15,12,25,6,67,10,2,3,1,6,10,2,6,2,9,1,9,4,4,1,2,16,2,
        5,9,2,3,8,1,8,3,9,4,8,6,4,8,11,3,2,1,1,3,26,1,7,5,1,11,1,5,3,5,2,13,6,39,5,1,5,2,11,6,10,5,1,15,5,3,6,19,21,22,2,4,1,6,1,8,1,4,8,2,4,2,2,9,2,
        1,1,1,4,3,6,3,12,7,1,14,2,4,10,2,13,1,17,7,3,2,1,3,2,13,7,14,12,3,1,29,2,8,9,15,14,9,14,1,3,1,6,5,9,11,3,38,43,20,7,7,8,5,15,12,19,15,81,8,7,
        1,5,73,13,37,28,8,8,1,15,18,20,165,28,1,6,11,8,4,14,7,15,1,3,3,6,4,1,7,14,1,1,11,30,1,5,1,4,14,1,4,2,7,52,2,6,29,3,1,9,1,21,3,5,1,26,3,11,14,
        11,1,17,5,1,2,1,3,2,8,1,2,9,12,1,1,2,3,8,3,24,12,7,7,5,17,3,3,3,1,23,10,4,4,6,3,1,16,17,22,3,10,21,16,16,6,4,10,2,1,1,2,8,8,6,5,3,3,3,39,25,
        15,1,1,16,6,7,25,15,6,6,12,1,22,13,1,4,9,5,12,2,9,1,12,28,8,3,5,10,22,60,1,2,40,4,61,63,4,1,13,12,1,4,31,12,1,14,89,5,16,6,29,14,2,5,49,18,18,
        5,29,33,47,1,17,1,19,12,2,9,7,39,12,3,7,12,39,3,1,46,4,12,3,8,9,5,31,15,18,3,2,2,66,19,13,17,5,3,46,124,13,57,34,2,5,4,5,8,1,1,1,4,3,1,17,5,
        3,5,3,1,8,5,6,3,27,3,26,7,12,7,2,17,3,7,18,78,16,4,36,1,2,1,6,2,1,39,17,7,4,13,4,4,4,1,10,4,2,4,6,3,10,1,19,1,26,2,4,33,2,73,47,7,3,8,2,4,15,
        18,1,29,2,41,14,1,21,16,41,7,39,25,13,44,2,2,10,1,13,7,1,7,3,5,20,4,8,2,49,1,10,6,1,6,7,10,7,11,16,3,12,20,4,10,3,1,2,11,2,28,9,2,4,7,2,15,1,
        27,1,28,17,4,5,10,7,3,24,10,11,6,26,3,2,7,2,2,49,16,10,16,15,4,5,27,61,30,14,38,22,2,7,5,1,3,12,23,24,17,17,3,3,2,4,1,6,2,7,5,1,1,5,1,1,9,4,
        1,3,6,1,8,2,8,4,14,3,5,11,4,1,3,32,1,19,4,1,13,11,5,2,1,8,6,8,1,6,5,13,3,23,11,5,3,16,3,9,10,1,24,3,198,52,4,2,2,5,14,5,4,22,5,20,4,11,6,41,
        1,5,2,2,11,5,2,28,35,8,22,3,18,3,10,7,5,3,4,1,5,3,8,9,3,6,2,16,22,4,5,5,3,3,18,23,2,6,23,5,27,8,1,33,2,12,43,16,5,2,3,6,1,20,4,2,9,7,1,11,2,
        10,3,14,31,9,3,25,18,20,2,5,5,26,14,1,11,17,12,40,19,9,6,31,83,2,7,9,19,78,12,14,21,76,12,113,79,34,4,1,1,61,18,85,10,2,2,13,31,11,50,6,33,159,
        179,6,6,7,4,4,2,4,2,5,8,7,20,32,22,1,3,10,6,7,28,5,10,9,2,77,19,13,2,5,1,4,4,7,4,13,3,9,31,17,3,26,2,6,6,5,4,1,7,11,3,4,2,1,6,2,20,4,1,9,2,6,
        3,7,1,1,1,20,2,3,1,6,2,3,6,2,4,8,1,5,13,8,4,11,23,1,10,6,2,1,3,21,2,2,4,24,31,4,10,10,2,5,192,15,4,16,7,9,51,1,2,1,1,5,1,1,2,1,3,5,3,1,3,4,1,
        3,1,3,3,9,8,1,2,2,2,4,4,18,12,92,2,10,4,3,14,5,25,16,42,4,14,4,2,21,5,126,30,31,2,1,5,13,3,22,5,6,6,20,12,1,14,12,87,3,19,1,8,2,9,9,3,3,23,2,
        3,7,6,3,1,2,3,9,1,3,1,6,3,2,1,3,11,3,1,6,10,3,2,3,1,2,1,5,1,1,11,3,6,4,1,7,2,1,2,5,5,34,4,14,18,4,19,7,5,8,2,6,79,1,5,2,14,8,2,9,2,1,36,28,16,
        4,1,1,1,2,12,6,42,39,16,23,7,15,15,3,2,12,7,21,64,6,9,28,8,12,3,3,41,59,24,51,55,57,294,9,9,2,6,2,15,1,2,13,38,90,9,9,9,3,11,7,1,1,1,5,6,3,2,
        1,2,2,3,8,1,4,4,1,5,7,1,4,3,20,4,9,1,1,1,5,5,17,1,5,2,6,2,4,1,4,5,7,3,18,11,11,32,7,5,4,7,11,127,8,4,3,3,1,10,1,1,6,21,14,1,16,1,7,1,3,6,9,65,
        51,4,3,13,3,10,1,1,12,9,21,110,3,19,24,1,1,10,62,4,1,29,42,78,28,20,18,82,6,3,15,6,84,58,253,15,155,264,15,21,9,14,7,58,40,39,
    };
    static ImWchar base_ranges[] = // not zero-terminated
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF  // Half-width characters
    };
    static ImWchar full_ranges[IM_ARRAYSIZE(base_ranges) + IM_ARRAYSIZE(accumulative_offsets_from_0x4E00)*2 + 1] = { 0 };
    if (!full_ranges[0])
    {
        memcpy(full_ranges, base_ranges, sizeof(base_ranges));
        UnpackAccumulativeOffsetsIntoRanges(0x4E00, accumulative_offsets_from_0x4E00, IM_ARRAYSIZE(accumulative_offsets_from_0x4E00), full_ranges + IM_ARRAYSIZE(base_ranges));
    }
    return &full_ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesCyrillic()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesThai()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin
        0x2010, 0x205E, // Punctuations
        0x0E00, 0x0E7F, // Thai
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesVietnamese()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin
        0x0102, 0x0103,
        0x0110, 0x0111,
        0x0128, 0x0129,
        0x0168, 0x0169,
        0x01A0, 0x01A1,
        0x01AF, 0x01B0,
        0x1EA0, 0x1EF9,
        0,
    };
    return &ranges[0];
}

const ImWchar*  ImFontAtlas::GetGlyphRangesArabic()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin
        0x0600, 0x06FF, // Arabic
        0x0750, 0x077F, // Arabic Supplement
        0x08A0, 0x08FF, // Arabic Extended-A
        0xFB50, 0xFDFF, // Arabic Presentation Forms-A
        0xFE70, 0xFEFF, // Arabic Presentation Forms-B
        0,
    };
    return &ranges[0];
}

const ImWchar* ImFontAtlas::GetGlyphRangesLatin()
{
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin
        0x0100, 0x017F, // Latin Extended-A
        0x0180, 0x024F, // Latin Extended-B
        0x1E00, 0x1EFF, // Latin Extended Additional
        0x2C60, 0x2C7F, // Latin Extended-C
        0xA720, 0xA7FF, // Latin Extended-D
        0xAB30, 0xAB6F, // Latin Extended-E
        0,
    };
    return &ranges[0];
}

//-----------------------------------------------------------------------------
// [SECTION] ImFontGlyphRangesBuilder
//-----------------------------------------------------------------------------

void ImFontGlyphRangesBuilder::AddText(const char* text, const char* text_end)
{
    while (text_end ? (text < text_end) : *text)
    {
        unsigned int c = 0;
        int c_len = ImTextCharFromUtf8(&c, text, text_end);
        text += c_len;
        if (c_len == 0)
            break;
        AddChar((ImWchar)c);
    }
}

void ImFontGlyphRangesBuilder::AddRanges(const ImWchar* ranges)
{
    for (; ranges[0]; ranges += 2)
        for (ImWchar c = ranges[0]; c <= ranges[1]; c++)
            AddChar(c);
}

void ImFontGlyphRangesBuilder::BuildRanges(ImVector<ImWchar>* out_ranges)
{
    const int max_codepoint = IM_UNICODE_CODEPOINT_MAX;
    for (int n = 0; n <= max_codepoint; n++)
        if (GetBit(n))
        {
            out_ranges->push_back((ImWchar)n);
            while (n < max_codepoint && GetBit(n + 1))
                n++;
            out_ranges->push_back((ImWchar)n);
        }
    out_ranges->push_back(0);
}

//-----------------------------------------------------------------------------
// [SECTION] ImFont
//-----------------------------------------------------------------------------

ImFont::ImFont()
{
    FontSize = 0.0f;
    FallbackAdvanceX = 0.0f;
    FallbackChar = (ImWchar)'?';
    EllipsisChar = (ImWchar)-1;
    DisplayOffset = ImVec2(0.0f, 0.0f);
    FallbackGlyph = NULL;
    ContainerAtlas = NULL;
    ConfigData = NULL;
    ConfigDataCount = 0;
    DirtyLookupTables = false;
    Scale = 1.0f;
    Ascent = Descent = 0.0f;
    MetricsTotalSurface = 0;
    memset(Used4kPagesMap, 0, sizeof(Used4kPagesMap));
}

ImFont::~ImFont()
{
    ClearOutputData();
}

void    ImFont::ClearOutputData()
{
    FontSize = 0.0f;
    FallbackAdvanceX = 0.0f;
    Glyphs.clear();
    IndexAdvanceX.clear();
    IndexLookup.clear();
    FallbackGlyph = NULL;
    ContainerAtlas = NULL;
    DirtyLookupTables = true;
    Ascent = Descent = 0.0f;
    MetricsTotalSurface = 0;
}

void ImFont::BuildLookupTable()
{
    int max_codepoint = 0;
    for (int i = 0; i != Glyphs.Size; i++)
        max_codepoint = ImMax(max_codepoint, (int)Glyphs[i].Codepoint);

    // Build lookup table
    IM_ASSERT(Glyphs.Size < 0xFFFF); // -1 is reserved
    IndexAdvanceX.clear();
    IndexLookup.clear();
    DirtyLookupTables = false;
    memset(Used4kPagesMap, 0, sizeof(Used4kPagesMap));
    GrowIndex(max_codepoint + 1);
    for (int i = 0; i < Glyphs.Size; i++)
    {
        int codepoint = (int)Glyphs[i].Codepoint;
        IndexAdvanceX[codepoint] = Glyphs[i].AdvanceX;
        IndexLookup[codepoint] = (ImWchar)i;

        // Mark 4K page as used
        const int page_n = codepoint / 4096;
        Used4kPagesMap[page_n >> 3] |= 1 << (page_n & 7);
    }

    // Create a glyph to handle TAB
    // FIXME: Needs proper TAB handling but it needs to be contextualized (or we could arbitrary say that each string starts at "column 0" ?)
    if (FindGlyph((ImWchar)' '))
    {
        if (Glyphs.back().Codepoint != '\t')   // So we can call this function multiple times (FIXME: Flaky)
            Glyphs.resize(Glyphs.Size + 1);
        ImFontGlyph& tab_glyph = Glyphs.back();
        tab_glyph = *FindGlyph((ImWchar)' ');
        tab_glyph.Codepoint = '\t';
        tab_glyph.AdvanceX *= IM_TABSIZE;
        IndexAdvanceX[(int)tab_glyph.Codepoint] = (float)tab_glyph.AdvanceX;
        IndexLookup[(int)tab_glyph.Codepoint] = (ImWchar)(Glyphs.Size - 1);
    }

    // Mark special glyphs as not visible (note that AddGlyph already mark as non-visible glyphs with zero-size polygons)
    SetGlyphVisible((ImWchar)' ', false);
    SetGlyphVisible((ImWchar)'\t', false);

    // Setup fall-backs
    FallbackGlyph = FindGlyphNoFallback(FallbackChar);
    FallbackAdvanceX = FallbackGlyph ? FallbackGlyph->AdvanceX : 0.0f;
    for (int i = 0; i < max_codepoint + 1; i++)
        if (IndexAdvanceX[i] < 0.0f)
            IndexAdvanceX[i] = FallbackAdvanceX;
}

// API is designed this way to avoid exposing the 4K page size
// e.g. use with IsGlyphRangeUnused(0, 255)
bool ImFont::IsGlyphRangeUnused(unsigned int c_begin, unsigned int c_last)
{
    unsigned int page_begin = (c_begin / 4096);
    unsigned int page_last = (c_last / 4096);
    for (unsigned int page_n = page_begin; page_n <= page_last; page_n++)
        if ((page_n >> 3) < sizeof(Used4kPagesMap))
            if (Used4kPagesMap[page_n >> 3] & (1 << (page_n & 7)))
                return false;
    return true;
}

void ImFont::SetGlyphVisible(ImWchar c, bool visible)
{
    if (ImFontGlyph* glyph = (ImFontGlyph*)(void*)FindGlyph((ImWchar)c))
        glyph->Visible = visible ? 1 : 0;
}

void ImFont::SetFallbackChar(ImWchar c)
{
    FallbackChar = c;
    BuildLookupTable();
}

void ImFont::GrowIndex(int new_size)
{
    IM_ASSERT(IndexAdvanceX.Size == IndexLookup.Size);
    if (new_size <= IndexLookup.Size)
        return;
    IndexAdvanceX.resize(new_size, -1.0f);
    IndexLookup.resize(new_size, (ImWchar)-1);
}

// x0/y0/x1/y1 are offset from the character upper-left layout position, in pixels. Therefore x0/y0 are often fairly close to zero.
// Not to be mistaken with texture coordinates, which are held by u0/v0/u1/v1 in normalized format (0.0..1.0 on each texture axis).
// 'cfg' is not necessarily == 'this->ConfigData' because multiple source fonts+configs can be used to build one target font.
void ImFont::AddGlyph(ImFontConfig* cfg, ImWchar codepoint, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, float advance_x)
{
    if (cfg != NULL)
    {
        // Clamp & recenter if needed
        const float advance_x_original = advance_x;
        advance_x = ImClamp(advance_x, cfg->GlyphMinAdvanceX, cfg->GlyphMaxAdvanceX);
        if (advance_x != advance_x_original)
        {
            float char_off_x = cfg->PixelSnapH ? ImFloor((advance_x - advance_x_original) * 0.5f) : (advance_x - advance_x_original) * 0.5f;
            x0 += char_off_x;
            x1 += char_off_x;
        }

        // Snap to pixel
        if (cfg->PixelSnapH)
            advance_x = IM_ROUND(advance_x);

        // Bake spacing
        advance_x += cfg->GlyphExtraSpacing.x;
    }

    Glyphs.resize(Glyphs.Size + 1);
    ImFontGlyph& glyph = Glyphs.back();
    glyph.Codepoint = (unsigned int)codepoint;
    glyph.Visible = (x0 != x1) && (y0 != y1);
    glyph.X0 = x0;
    glyph.Y0 = y0;
    glyph.X1 = x1;
    glyph.Y1 = y1;
    glyph.U0 = u0;
    glyph.V0 = v0;
    glyph.U1 = u1;
    glyph.V1 = v1;
    glyph.AdvanceX = advance_x;

    // Compute rough surface usage metrics (+1 to account for average padding, +0.99 to round)
    // We use (U1-U0)*TexWidth instead of X1-X0 to account for oversampling.
    float pad = ContainerAtlas->TexGlyphPadding + 0.99f;
    DirtyLookupTables = true;
    MetricsTotalSurface += (int)((glyph.U1 - glyph.U0) * ContainerAtlas->TexWidth + pad) * (int)((glyph.V1 - glyph.V0) * ContainerAtlas->TexHeight + pad);
}

void ImFont::AddRemapChar(ImWchar dst, ImWchar src, bool overwrite_dst)
{
    IM_ASSERT(IndexLookup.Size > 0);    // Currently this can only be called AFTER the font has been built, aka after calling ImFontAtlas::GetTexDataAs*() function.
    unsigned int index_size = (unsigned int)IndexLookup.Size;

    if (dst < index_size && IndexLookup.Data[dst] == (ImWchar)-1 && !overwrite_dst) // 'dst' already exists
        return;
    if (src >= index_size && dst >= index_size) // both 'dst' and 'src' don't exist -> no-op
        return;

    GrowIndex(dst + 1);
    IndexLookup[dst] = (src < index_size) ? IndexLookup.Data[src] : (ImWchar)-1;
    IndexAdvanceX[dst] = (src < index_size) ? IndexAdvanceX.Data[src] : 1.0f;
}

const ImFontGlyph* ImFont::FindGlyph(ImWchar c) const
{
    if (c >= (size_t)IndexLookup.Size)
        return FallbackGlyph;
    const ImWchar i = IndexLookup.Data[c];
    if (i == (ImWchar)-1)
        return FallbackGlyph;
    return &Glyphs.Data[i];
}

const ImFontGlyph* ImFont::FindGlyphNoFallback(ImWchar c) const
{
    if (c >= (size_t)IndexLookup.Size)
        return NULL;
    const ImWchar i = IndexLookup.Data[c];
    if (i == (ImWchar)-1)
        return NULL;
    return &Glyphs.Data[i];
}

const char* ImFont::CalcWordWrapPositionA(float scale, const char* text, const char* text_end, float wrap_width) const
{
    // Simple word-wrapping for English, not full-featured. Please submit failing cases!
    // FIXME: Much possible improvements (don't cut things like "word !", "word!!!" but cut within "word,,,,", more sensible support for punctuations, support for Unicode punctuations, etc.)

    // For references, possible wrap point marked with ^
    //  "aaa bbb, ccc,ddd. eee   fff. ggg!"
    //      ^    ^    ^   ^   ^__    ^    ^

    // List of hardcoded separators: .,;!?'"

    // Skip extra blanks after a line returns (that includes not counting them in width computation)
    // e.g. "Hello    world" --> "Hello" "World"

    // Cut words that cannot possibly fit within one line.
    // e.g.: "The tropical fish" with ~5 characters worth of width --> "The tr" "opical" "fish"

    float line_width = 0.0f;
    float word_width = 0.0f;
    float blank_width = 0.0f;
    wrap_width /= scale; // We work with unscaled widths to avoid scaling every characters

    const char* word_end = text;
    const char* prev_word_end = NULL;
    bool inside_word = true;

    const char* s = text;
    while (s < text_end)
    {
        unsigned int c = (unsigned int)*s;
        const char* next_s;
        if (c < 0x80)
            next_s = s + 1;
        else
            next_s = s + ImTextCharFromUtf8(&c, s, text_end);
        if (c == 0)
            break;

        if (c < 32)
        {
            if (c == '\n')
            {
                line_width = word_width = blank_width = 0.0f;
                inside_word = true;
                s = next_s;
                continue;
            }
            if (c == '\r')
            {
                s = next_s;
                continue;
            }
        }

        const float char_width = ((int)c < IndexAdvanceX.Size ? IndexAdvanceX.Data[c] : FallbackAdvanceX);
        if (ImCharIsBlankW(c))
        {
            if (inside_word)
            {
                line_width += blank_width;
                blank_width = 0.0f;
                word_end = s;
            }
            blank_width += char_width;
            inside_word = false;
        }
        else
        {
            word_width += char_width;
            if (inside_word)
            {
                word_end = next_s;
            }
            else
            {
                prev_word_end = word_end;
                line_width += word_width + blank_width;
                word_width = blank_width = 0.0f;
            }

            // Allow wrapping after punctuation.
            inside_word = (c != '.' && c != ',' && c != ';' && c != '!' && c != '?' && c != '\"');
        }

        // We ignore blank width at the end of the line (they can be skipped)
        if (line_width + word_width > wrap_width)
        {
            // Words that cannot possibly fit within an entire line will be cut anywhere.
            if (word_width < wrap_width)
                s = prev_word_end ? prev_word_end : word_end;
            break;
        }

        s = next_s;
    }

    return s;
}

ImVec2 ImFont::CalcTextSizeA(float size, float max_width, float wrap_width, const char* text_begin, const char* text_end, const char** remaining) const
{
    if (!text_end)
        text_end = text_begin + strlen(text_begin); // FIXME-OPT: Need to avoid this.

    const float line_height = size;
    const float scale = size / FontSize;

    ImVec2 text_size = ImVec2(0, 0);
    float line_width = 0.0f;

    const bool word_wrap_enabled = (wrap_width > 0.0f);
    const char* word_wrap_eol = NULL;

    const char* s = text_begin;
    while (s < text_end)
    {
        if (word_wrap_enabled)
        {
            // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
            if (!word_wrap_eol)
            {
                word_wrap_eol = CalcWordWrapPositionA(scale, s, text_end, wrap_width - line_width);
                if (word_wrap_eol == s) // Wrap_width is too small to fit anything. Force displaying 1 character to minimize the height discontinuity.
                    word_wrap_eol++;    // +1 may not be a character start point in UTF-8 but it's ok because we use s >= word_wrap_eol below
            }

            if (s >= word_wrap_eol)
            {
                if (text_size.x < line_width)
                    text_size.x = line_width;
                text_size.y += line_height;
                line_width = 0.0f;
                word_wrap_eol = NULL;

                // Wrapping skips upcoming blanks
                while (s < text_end)
                {
                    const char c = *s;
                    if (ImCharIsBlankA(c)) { s++; } else if (c == '\n') { s++; break; } else { break; }
                }
                continue;
            }
        }

        // Decode and advance source
        const char* prev_s = s;
        unsigned int c = (unsigned int)*s;
        if (c < 0x80)
        {
            s += 1;
        }
        else
        {
            s += ImTextCharFromUtf8(&c, s, text_end);
            if (c == 0) // Malformed UTF-8?
                break;
        }

        if (c < 32)
        {
            if (c == '\n')
            {
                text_size.x = ImMax(text_size.x, line_width);
                text_size.y += line_height;
                line_width = 0.0f;
                continue;
            }
            if (c == '\r')
                continue;
        }

        const float char_width = ((int)c < IndexAdvanceX.Size ? IndexAdvanceX.Data[c] : FallbackAdvanceX) * scale;
        if (line_width + char_width >= max_width)
        {
            s = prev_s;
            break;
        }

        line_width += char_width;
    }

    if (text_size.x < line_width)
        text_size.x = line_width;

    if (line_width > 0 || text_size.y == 0.0f)
        text_size.y += line_height;

    if (remaining)
        *remaining = s;

    return text_size;
}

void ImFont::RenderChar(ImDrawList* draw_list, float size, ImVec2 pos, ImU32 col, ImWchar c) const
{
    const ImFontGlyph* glyph = FindGlyph(c);
    if (!glyph || !glyph->Visible)
        return;
    float scale = (size >= 0.0f) ? (size / FontSize) : 1.0f;
    pos.x = IM_FLOOR(pos.x + DisplayOffset.x);
    pos.y = IM_FLOOR(pos.y + DisplayOffset.y);
    draw_list->PrimReserve(6, 4);
    draw_list->PrimRectUV(ImVec2(pos.x + glyph->X0 * scale, pos.y + glyph->Y0 * scale), ImVec2(pos.x + glyph->X1 * scale, pos.y + glyph->Y1 * scale), ImVec2(glyph->U0, glyph->V0), ImVec2(glyph->U1, glyph->V1), col);
}

void ImFont::RenderText(ImDrawList* draw_list, float size, ImVec2 pos, ImU32 col, const ImVec4& clip_rect, const char* text_begin, const char* text_end, float wrap_width, bool cpu_fine_clip) const
{
    if (!text_end)
        text_end = text_begin + strlen(text_begin); // ImGui:: functions generally already provides a valid text_end, so this is merely to handle direct calls.

    // Align to be pixel perfect
    pos.x = IM_FLOOR(pos.x + DisplayOffset.x);
    pos.y = IM_FLOOR(pos.y + DisplayOffset.y);
    float x = pos.x;
    float y = pos.y;
    if (y > clip_rect.w)
        return;

    const float scale = size / FontSize;
    const float line_height = FontSize * scale;
    const bool word_wrap_enabled = (wrap_width > 0.0f);
    const char* word_wrap_eol = NULL;

    // Fast-forward to first visible line
    const char* s = text_begin;
    if (y + line_height < clip_rect.y && !word_wrap_enabled)
        while (y + line_height < clip_rect.y && s < text_end)
        {
            s = (const char*)memchr(s, '\n', text_end - s);
            s = s ? s + 1 : text_end;
            y += line_height;
        }

    // For large text, scan for the last visible line in order to avoid over-reserving in the call to PrimReserve()
    // Note that very large horizontal line will still be affected by the issue (e.g. a one megabyte string buffer without a newline will likely crash atm)
    if (text_end - s > 10000 && !word_wrap_enabled)
    {
        const char* s_end = s;
        float y_end = y;
        while (y_end < clip_rect.w && s_end < text_end)
        {
            s_end = (const char*)memchr(s_end, '\n', text_end - s_end);
            s_end = s_end ? s_end + 1 : text_end;
            y_end += line_height;
        }
        text_end = s_end;
    }
    if (s == text_end)
        return;

    // Reserve vertices for remaining worse case (over-reserving is useful and easily amortized)
    const int vtx_count_max = (int)(text_end - s) * 4;
    const int idx_count_max = (int)(text_end - s) * 6;
    const int idx_expected_size = draw_list->IdxBuffer.Size + idx_count_max;
    draw_list->PrimReserve(idx_count_max, vtx_count_max);

    ImDrawVert* vtx_write = draw_list->_VtxWritePtr;
    ImDrawIdx* idx_write = draw_list->_IdxWritePtr;
    unsigned int vtx_current_idx = draw_list->_VtxCurrentIdx;

    while (s < text_end)
    {
        if (word_wrap_enabled)
        {
            // Calculate how far we can render. Requires two passes on the string data but keeps the code simple and not intrusive for what's essentially an uncommon feature.
            if (!word_wrap_eol)
            {
                word_wrap_eol = CalcWordWrapPositionA(scale, s, text_end, wrap_width - (x - pos.x));
                if (word_wrap_eol == s) // Wrap_width is too small to fit anything. Force displaying 1 character to minimize the height discontinuity.
                    word_wrap_eol++;    // +1 may not be a character start point in UTF-8 but it's ok because we use s >= word_wrap_eol below
            }

            if (s >= word_wrap_eol)
            {
                x = pos.x;
                y += line_height;
                word_wrap_eol = NULL;

                // Wrapping skips upcoming blanks
                while (s < text_end)
                {
                    const char c = *s;
                    if (ImCharIsBlankA(c)) { s++; } else if (c == '\n') { s++; break; } else { break; }
                }
                continue;
            }
        }

        // Decode and advance source
        unsigned int c = (unsigned int)*s;
        if (c < 0x80)
        {
            s += 1;
        }
        else
        {
            s += ImTextCharFromUtf8(&c, s, text_end);
            if (c == 0) // Malformed UTF-8?
                break;
        }

        if (c < 32)
        {
            if (c == '\n')
            {
                x = pos.x;
                y += line_height;
                if (y > clip_rect.w)
                    break; // break out of main loop
                continue;
            }
            if (c == '\r')
                continue;
        }

        const ImFontGlyph* glyph = FindGlyph((ImWchar)c);
        if (glyph == NULL)
            continue;

        float char_width = glyph->AdvanceX * scale;
        if (glyph->Visible)
        {
            // We don't do a second finer clipping test on the Y axis as we've already skipped anything before clip_rect.y and exit once we pass clip_rect.w
            float x1 = x + glyph->X0 * scale;
            float x2 = x + glyph->X1 * scale;
            float y1 = y + glyph->Y0 * scale;
            float y2 = y + glyph->Y1 * scale;
            if (x1 <= clip_rect.z && x2 >= clip_rect.x)
            {
                // Render a character
                float u1 = glyph->U0;
                float v1 = glyph->V0;
                float u2 = glyph->U1;
                float v2 = glyph->V1;

                // CPU side clipping used to fit text in their frame when the frame is too small. Only does clipping for axis aligned quads.
                if (cpu_fine_clip)
                {
                    if (x1 < clip_rect.x)
                    {
                        u1 = u1 + (1.0f - (x2 - clip_rect.x) / (x2 - x1)) * (u2 - u1);
                        x1 = clip_rect.x;
                    }
                    if (y1 < clip_rect.y)
                    {
                        v1 = v1 + (1.0f - (y2 - clip_rect.y) / (y2 - y1)) * (v2 - v1);
                        y1 = clip_rect.y;
                    }
                    if (x2 > clip_rect.z)
                    {
                        u2 = u1 + ((clip_rect.z - x1) / (x2 - x1)) * (u2 - u1);
                        x2 = clip_rect.z;
                    }
                    if (y2 > clip_rect.w)
                    {
                        v2 = v1 + ((clip_rect.w - y1) / (y2 - y1)) * (v2 - v1);
                        y2 = clip_rect.w;
                    }
                    if (y1 >= y2)
                    {
                        x += char_width;
                        continue;
                    }
                }

                // We are NOT calling PrimRectUV() here because non-inlined causes too much overhead in a debug builds. Inlined here:
                {
                    idx_write[0] = (ImDrawIdx)(vtx_current_idx); idx_write[1] = (ImDrawIdx)(vtx_current_idx+1); idx_write[2] = (ImDrawIdx)(vtx_current_idx+2);
                    idx_write[3] = (ImDrawIdx)(vtx_current_idx); idx_write[4] = (ImDrawIdx)(vtx_current_idx+2); idx_write[5] = (ImDrawIdx)(vtx_current_idx+3);
                    vtx_write[0].pos.x = x1; vtx_write[0].pos.y = y1; vtx_write[0].col = col; vtx_write[0].uv.x = u1; vtx_write[0].uv.y = v1;
                    vtx_write[1].pos.x = x2; vtx_write[1].pos.y = y1; vtx_write[1].col = col; vtx_write[1].uv.x = u2; vtx_write[1].uv.y = v1;
                    vtx_write[2].pos.x = x2; vtx_write[2].pos.y = y2; vtx_write[2].col = col; vtx_write[2].uv.x = u2; vtx_write[2].uv.y = v2;
                    vtx_write[3].pos.x = x1; vtx_write[3].pos.y = y2; vtx_write[3].col = col; vtx_write[3].uv.x = u1; vtx_write[3].uv.y = v2;
                    vtx_write += 4;
                    vtx_current_idx += 4;
                    idx_write += 6;
                }
            }
        }
        x += char_width;
    }

    // Give back unused vertices (clipped ones, blanks) ~ this is essentially a PrimUnreserve() action.
    draw_list->VtxBuffer.Size = (int)(vtx_write - draw_list->VtxBuffer.Data); // Same as calling shrink()
    draw_list->IdxBuffer.Size = (int)(idx_write - draw_list->IdxBuffer.Data);
    draw_list->CmdBuffer[draw_list->CmdBuffer.Size - 1].ElemCount -= (idx_expected_size - draw_list->IdxBuffer.Size);
    draw_list->_VtxWritePtr = vtx_write;
    draw_list->_IdxWritePtr = idx_write;
    draw_list->_VtxCurrentIdx = vtx_current_idx;
}

//-----------------------------------------------------------------------------
// [SECTION] ImGui Internal Render Helpers
//-----------------------------------------------------------------------------
// Vaguely redesigned to stop accessing ImGui global state:
// - RenderArrow()
// - RenderBullet()
// - RenderCheckMark()
// - RenderMouseCursor()
// - RenderArrowDockMenu()
// - RenderArrowPointingAt()
// - RenderRectFilledRangeH()
// - RenderRectFilledWithHole()
//-----------------------------------------------------------------------------
// Function in need of a redesign (legacy mess)
// - RenderColorRectWithAlphaCheckerboard()
//-----------------------------------------------------------------------------

// Render an arrow aimed to be aligned with text (p_min is a position in the same space text would be positioned). To e.g. denote expanded/collapsed state
void ImGui::RenderArrow(ImDrawList* draw_list, ImVec2 pos, ImU32 col, ImGuiDir dir, float scale)
{
    const float h = draw_list->_Data->FontSize * 1.00f;
    float r = h * 0.40f * scale;
    ImVec2 center = pos + ImVec2(h * 0.50f, h * 0.50f * scale);

    ImVec2 a, b, c;
    switch (dir)
    {
    case ImGuiDir_Up:
    case ImGuiDir_Down:
        if (dir == ImGuiDir_Up) r = -r;
        a = ImVec2(+0.000f, +0.750f) * r;
        b = ImVec2(-0.866f, -0.750f) * r;
        c = ImVec2(+0.866f, -0.750f) * r;
        break;
    case ImGuiDir_Left:
    case ImGuiDir_Right:
        if (dir == ImGuiDir_Left) r = -r;
        a = ImVec2(+0.750f, +0.000f) * r;
        b = ImVec2(-0.750f, +0.866f) * r;
        c = ImVec2(-0.750f, -0.866f) * r;
        break;
    case ImGuiDir_None:
    case ImGuiDir_COUNT:
        IM_ASSERT(0);
        break;
    }
    draw_list->AddTriangleFilled(center + a, center + b, center + c, col);
}

void ImGui::RenderBullet(ImDrawList* draw_list, ImVec2 pos, ImU32 col)
{
    draw_list->AddCircleFilled(pos, draw_list->_Data->FontSize * 0.20f, col, 8);
}

void ImGui::RenderCheckMark(ImDrawList* draw_list, ImVec2 pos, ImU32 col, float sz)
{
    float thickness = ImMax(sz / 5.0f, 1.0f);
    sz -= thickness * 0.5f;
    pos += ImVec2(thickness * 0.25f, thickness * 0.25f);

    float third = sz / 3.0f;
    float bx = pos.x + third;
    float by = pos.y + sz - third * 0.5f;
    draw_list->PathLineTo(ImVec2(bx - third, by - third));
    draw_list->PathLineTo(ImVec2(bx, by));
    draw_list->PathLineTo(ImVec2(bx + third * 2.0f, by - third * 2.0f));
    draw_list->PathStroke(col, false, thickness);
}

void ImGui::RenderMouseCursor(ImDrawList* draw_list, ImVec2 pos, float scale, ImGuiMouseCursor mouse_cursor, ImU32 col_fill, ImU32 col_border, ImU32 col_shadow)
{
    if (mouse_cursor == ImGuiMouseCursor_None)
        return;
    IM_ASSERT(mouse_cursor > ImGuiMouseCursor_None && mouse_cursor < ImGuiMouseCursor_COUNT);

    ImFontAtlas* font_atlas = draw_list->_Data->Font->ContainerAtlas;
    ImVec2 offset, size, uv[4];
    if (font_atlas->GetMouseCursorTexData(mouse_cursor, &offset, &size, &uv[0], &uv[2]))
    {
        pos -= offset;
        const ImTextureID tex_id = font_atlas->TexID;
        draw_list->PushTextureID(tex_id);
        draw_list->AddImage(tex_id, pos + ImVec2(1, 0) * scale, pos + (ImVec2(1, 0) + size) * scale,    uv[2], uv[3], col_shadow);
        draw_list->AddImage(tex_id, pos + ImVec2(2, 0) * scale, pos + (ImVec2(2, 0) + size) * scale,    uv[2], uv[3], col_shadow);
        draw_list->AddImage(tex_id, pos,                        pos + size * scale,                     uv[2], uv[3], col_border);
        draw_list->AddImage(tex_id, pos,                        pos + size * scale,                     uv[0], uv[1], col_fill);
        draw_list->PopTextureID();
    }
}

// Render an arrow. 'pos' is position of the arrow tip. half_sz.x is length from base to tip. half_sz.y is length on each side.
void ImGui::RenderArrowPointingAt(ImDrawList* draw_list, ImVec2 pos, ImVec2 half_sz, ImGuiDir direction, ImU32 col)
{
    switch (direction)
    {
    case ImGuiDir_Left:  draw_list->AddTriangleFilled(ImVec2(pos.x + half_sz.x, pos.y - half_sz.y), ImVec2(pos.x + half_sz.x, pos.y + half_sz.y), pos, col); return;
    case ImGuiDir_Right: draw_list->AddTriangleFilled(ImVec2(pos.x - half_sz.x, pos.y + half_sz.y), ImVec2(pos.x - half_sz.x, pos.y - half_sz.y), pos, col); return;
    case ImGuiDir_Up:    draw_list->AddTriangleFilled(ImVec2(pos.x + half_sz.x, pos.y + half_sz.y), ImVec2(pos.x - half_sz.x, pos.y + half_sz.y), pos, col); return;
    case ImGuiDir_Down:  draw_list->AddTriangleFilled(ImVec2(pos.x - half_sz.x, pos.y - half_sz.y), ImVec2(pos.x + half_sz.x, pos.y - half_sz.y), pos, col); return;
    case ImGuiDir_None: case ImGuiDir_COUNT: break; // Fix warnings
    }
}

// This is less wide than RenderArrow() and we use in dock nodes instead of the regular RenderArrow() to denote a change of functionality,
// and because the saved space means that the left-most tab label can stay at exactly the same position as the label of a loose window.
void ImGui::RenderArrowDockMenu(ImDrawList* draw_list, ImVec2 p_min, float sz, ImU32 col)
{
    draw_list->AddRectFilled(p_min + ImVec2(sz * 0.10f, sz * 0.15f), p_min + ImVec2(sz * 0.70f, sz * 0.30f), col);
    RenderArrowPointingAt(draw_list, p_min + ImVec2(sz * 0.40f, sz * 0.85f), ImVec2(sz * 0.30f, sz * 0.40f), ImGuiDir_Down, col);
}

static inline float ImAcos01(float x)
{
    if (x <= 0.0f) return IM_PI * 0.5f;
    if (x >= 1.0f) return 0.0f;
    return ImAcos(x);
    //return (-0.69813170079773212f * x * x - 0.87266462599716477f) * x + 1.5707963267948966f; // Cheap approximation, may be enough for what we do.
}

// FIXME: Cleanup and move code to ImDrawList.
void ImGui::RenderRectFilledRangeH(ImDrawList* draw_list, const ImRect& rect, ImU32 col, float x_start_norm, float x_end_norm, float rounding)
{
    if (x_end_norm == x_start_norm)
        return;
    if (x_start_norm > x_end_norm)
        ImSwap(x_start_norm, x_end_norm);

    ImVec2 p0 = ImVec2(ImLerp(rect.Min.x, rect.Max.x, x_start_norm), rect.Min.y);
    ImVec2 p1 = ImVec2(ImLerp(rect.Min.x, rect.Max.x, x_end_norm), rect.Max.y);
    if (rounding == 0.0f)
    {
        draw_list->AddRectFilled(p0, p1, col, 0.0f);
        return;
    }

    rounding = ImClamp(ImMin((rect.Max.x - rect.Min.x) * 0.5f, (rect.Max.y - rect.Min.y) * 0.5f) - 1.0f, 0.0f, rounding);
    const float inv_rounding = 1.0f / rounding;
    const float arc0_b = ImAcos01(1.0f - (p0.x - rect.Min.x) * inv_rounding);
    const float arc0_e = ImAcos01(1.0f - (p1.x - rect.Min.x) * inv_rounding);
    const float half_pi = IM_PI * 0.5f; // We will == compare to this because we know this is the exact value ImAcos01 can return.
    const float x0 = ImMax(p0.x, rect.Min.x + rounding);
    if (arc0_b == arc0_e)
    {
        draw_list->PathLineTo(ImVec2(x0, p1.y));
        draw_list->PathLineTo(ImVec2(x0, p0.y));
    }
    else if (arc0_b == 0.0f && arc0_e == half_pi)
    {
        draw_list->PathArcToFast(ImVec2(x0, p1.y - rounding), rounding, 3, 6); // BL
        draw_list->PathArcToFast(ImVec2(x0, p0.y + rounding), rounding, 6, 9); // TR
    }
    else
    {
        draw_list->PathArcTo(ImVec2(x0, p1.y - rounding), rounding, IM_PI - arc0_e, IM_PI - arc0_b, 3); // BL
        draw_list->PathArcTo(ImVec2(x0, p0.y + rounding), rounding, IM_PI + arc0_b, IM_PI + arc0_e, 3); // TR
    }
    if (p1.x > rect.Min.x + rounding)
    {
        const float arc1_b = ImAcos01(1.0f - (rect.Max.x - p1.x) * inv_rounding);
        const float arc1_e = ImAcos01(1.0f - (rect.Max.x - p0.x) * inv_rounding);
        const float x1 = ImMin(p1.x, rect.Max.x - rounding);
        if (arc1_b == arc1_e)
        {
            draw_list->PathLineTo(ImVec2(x1, p0.y));
            draw_list->PathLineTo(ImVec2(x1, p1.y));
        }
        else if (arc1_b == 0.0f && arc1_e == half_pi)
        {
            draw_list->PathArcToFast(ImVec2(x1, p0.y + rounding), rounding, 9, 12); // TR
            draw_list->PathArcToFast(ImVec2(x1, p1.y - rounding), rounding, 0, 3);  // BR
        }
        else
        {
            draw_list->PathArcTo(ImVec2(x1, p0.y + rounding), rounding, -arc1_e, -arc1_b, 3); // TR
            draw_list->PathArcTo(ImVec2(x1, p1.y - rounding), rounding, +arc1_b, +arc1_e, 3); // BR
        }
    }
    draw_list->PathFillConvex(col);
}

void ImGui::RenderRectFilledWithHole(ImDrawList* draw_list, ImRect outer, ImRect inner, ImU32 col, float rounding)
{
    const bool fill_L = (inner.Min.x > outer.Min.x);
    const bool fill_R = (inner.Max.x < outer.Max.x);
    const bool fill_U = (inner.Min.y > outer.Min.y);
    const bool fill_D = (inner.Max.y < outer.Max.y);
    if (fill_L) draw_list->AddRectFilled(ImVec2(outer.Min.x, inner.Min.y), ImVec2(inner.Min.x, inner.Max.y), col, rounding, (fill_U ? 0 : ImDrawCornerFlags_TopLeft) | (fill_D ? 0 : ImDrawCornerFlags_BotLeft));
    if (fill_R) draw_list->AddRectFilled(ImVec2(inner.Max.x, inner.Min.y), ImVec2(outer.Max.x, inner.Max.y), col, rounding, (fill_U ? 0 : ImDrawCornerFlags_TopRight) | (fill_D ? 0 : ImDrawCornerFlags_BotRight));
    if (fill_U) draw_list->AddRectFilled(ImVec2(inner.Min.x, outer.Min.y), ImVec2(inner.Max.x, inner.Min.y), col, rounding, (fill_L ? 0 : ImDrawCornerFlags_TopLeft) | (fill_R ? 0 : ImDrawCornerFlags_TopRight));
    if (fill_D) draw_list->AddRectFilled(ImVec2(inner.Min.x, inner.Max.y), ImVec2(inner.Max.x, outer.Max.y), col, rounding, (fill_L ? 0 : ImDrawCornerFlags_BotLeft) | (fill_R ? 0 : ImDrawCornerFlags_BotRight));
    if (fill_L && fill_U) draw_list->AddRectFilled(ImVec2(outer.Min.x, outer.Min.y), ImVec2(inner.Min.x, inner.Min.y), col, rounding, ImDrawCornerFlags_TopLeft);
    if (fill_R && fill_U) draw_list->AddRectFilled(ImVec2(inner.Max.x, outer.Min.y), ImVec2(outer.Max.x, inner.Min.y), col, rounding, ImDrawCornerFlags_TopRight);
    if (fill_L && fill_D) draw_list->AddRectFilled(ImVec2(outer.Min.x, inner.Max.y), ImVec2(inner.Min.x, outer.Max.y), col, rounding, ImDrawCornerFlags_BotLeft);
    if (fill_R && fill_D) draw_list->AddRectFilled(ImVec2(inner.Max.x, inner.Max.y), ImVec2(outer.Max.x, outer.Max.y), col, rounding, ImDrawCornerFlags_BotRight);
}

// Helper for ColorPicker4()
// NB: This is rather brittle and will show artifact when rounding this enabled if rounded corners overlap multiple cells. Caller currently responsible for avoiding that.
// Spent a non reasonable amount of time trying to getting this right for ColorButton with rounding+anti-aliasing+ImGuiColorEditFlags_HalfAlphaPreview flag + various grid sizes and offsets, and eventually gave up... probably more reasonable to disable rounding altogether.
// FIXME: uses ImGui::GetColorU32
void ImGui::RenderColorRectWithAlphaCheckerboard(ImDrawList* draw_list, ImVec2 p_min, ImVec2 p_max, ImU32 col, float grid_step, ImVec2 grid_off, float rounding, int rounding_corners_flags)
{
    if (((col & IM_COL32_A_MASK) >> IM_COL32_A_SHIFT) < 0xFF)
    {
        ImU32 col_bg1 = ImGui::GetColorU32(ImAlphaBlendColors(IM_COL32(204, 204, 204, 255), col));
        ImU32 col_bg2 = ImGui::GetColorU32(ImAlphaBlendColors(IM_COL32(128, 128, 128, 255), col));
        draw_list->AddRectFilled(p_min, p_max, col_bg1, rounding, rounding_corners_flags);

        int yi = 0;
        for (float y = p_min.y + grid_off.y; y < p_max.y; y += grid_step, yi++)
        {
            float y1 = ImClamp(y, p_min.y, p_max.y), y2 = ImMin(y + grid_step, p_max.y);
            if (y2 <= y1)
                continue;
            for (float x = p_min.x + grid_off.x + (yi & 1) * grid_step; x < p_max.x; x += grid_step * 2.0f)
            {
                float x1 = ImClamp(x, p_min.x, p_max.x), x2 = ImMin(x + grid_step, p_max.x);
                if (x2 <= x1)
                    continue;
                int rounding_corners_flags_cell = 0;
                if (y1 <= p_min.y) { if (x1 <= p_min.x) rounding_corners_flags_cell |= ImDrawCornerFlags_TopLeft; if (x2 >= p_max.x) rounding_corners_flags_cell |= ImDrawCornerFlags_TopRight; }
                if (y2 >= p_max.y) { if (x1 <= p_min.x) rounding_corners_flags_cell |= ImDrawCornerFlags_BotLeft; if (x2 >= p_max.x) rounding_corners_flags_cell |= ImDrawCornerFlags_BotRight; }
                rounding_corners_flags_cell &= rounding_corners_flags;
                draw_list->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), col_bg2, rounding_corners_flags_cell ? rounding : 0.0f, rounding_corners_flags_cell);
            }
        }
    }
    else
    {
        draw_list->AddRectFilled(p_min, p_max, col, rounding, rounding_corners_flags);
    }
}

//-----------------------------------------------------------------------------
// [SECTION] Decompression code
//-----------------------------------------------------------------------------
// Compressed with stb_compress() then converted to a C array and encoded as base85.
// Use the program in misc/fonts/binary_to_compressed_c.cpp to create the array from a TTF file.
// The purpose of encoding as base85 instead of "0x00,0x01,..." style is only save on _source code_ size.
// Decompression from stb.h (public domain) by Sean Barrett https://github.com/nothings/stb/blob/master/stb.h
//-----------------------------------------------------------------------------

static unsigned int stb_decompress_length(const unsigned char *input)
{
    return (input[8] << 24) + (input[9] << 16) + (input[10] << 8) + input[11];
}

static unsigned char *stb__barrier_out_e, *stb__barrier_out_b;
static const unsigned char *stb__barrier_in_b;
static unsigned char *stb__dout;
static void stb__match(const unsigned char *data, unsigned int length)
{
    // INVERSE of memmove... write each byte before copying the next...
    IM_ASSERT(stb__dout + length <= stb__barrier_out_e);
    if (stb__dout + length > stb__barrier_out_e) { stb__dout += length; return; }
    if (data < stb__barrier_out_b) { stb__dout = stb__barrier_out_e+1; return; }
    while (length--) *stb__dout++ = *data++;
}

static void stb__lit(const unsigned char *data, unsigned int length)
{
    IM_ASSERT(stb__dout + length <= stb__barrier_out_e);
    if (stb__dout + length > stb__barrier_out_e) { stb__dout += length; return; }
    if (data < stb__barrier_in_b) { stb__dout = stb__barrier_out_e+1; return; }
    memcpy(stb__dout, data, length);
    stb__dout += length;
}

#define stb__in2(x)   ((i[x] << 8) + i[(x)+1])
#define stb__in3(x)   ((i[x] << 16) + stb__in2((x)+1))
#define stb__in4(x)   ((i[x] << 24) + stb__in3((x)+1))

static const unsigned char *stb_decompress_token(const unsigned char *i)
{
    if (*i >= 0x20) { // use fewer if's for cases that expand small
        if (*i >= 0x80)       stb__match(stb__dout-i[1]-1, i[0] - 0x80 + 1), i += 2;
        else if (*i >= 0x40)  stb__match(stb__dout-(stb__in2(0) - 0x4000 + 1), i[2]+1), i += 3;
        else /* *i >= 0x20 */ stb__lit(i+1, i[0] - 0x20 + 1), i += 1 + (i[0] - 0x20 + 1);
    } else { // more ifs for cases that expand large, since overhead is amortized
        if (*i >= 0x18)       stb__match(stb__dout-(stb__in3(0) - 0x180000 + 1), i[3]+1), i += 4;
        else if (*i >= 0x10)  stb__match(stb__dout-(stb__in3(0) - 0x100000 + 1), stb__in2(3)+1), i += 5;
        else if (*i >= 0x08)  stb__lit(i+2, stb__in2(0) - 0x0800 + 1), i += 2 + (stb__in2(0) - 0x0800 + 1);
        else if (*i == 0x07)  stb__lit(i+3, stb__in2(1) + 1), i += 3 + (stb__in2(1) + 1);
        else if (*i == 0x06)  stb__match(stb__dout-(stb__in3(1)+1), i[4]+1), i += 5;
        else if (*i == 0x04)  stb__match(stb__dout-(stb__in3(1)+1), stb__in2(4)+1), i += 6;
    }
    return i;
}

static unsigned int stb_adler32(unsigned int adler32, unsigned char *buffer, unsigned int buflen)
{
    const unsigned long ADLER_MOD = 65521;
    unsigned long s1 = adler32 & 0xffff, s2 = adler32 >> 16;
    unsigned long blocklen = buflen % 5552;

    unsigned long i;
    while (buflen) {
        for (i=0; i + 7 < blocklen; i += 8) {
            s1 += buffer[0], s2 += s1;
            s1 += buffer[1], s2 += s1;
            s1 += buffer[2], s2 += s1;
            s1 += buffer[3], s2 += s1;
            s1 += buffer[4], s2 += s1;
            s1 += buffer[5], s2 += s1;
            s1 += buffer[6], s2 += s1;
            s1 += buffer[7], s2 += s1;

            buffer += 8;
        }

        for (; i < blocklen; ++i)
            s1 += *buffer++, s2 += s1;

        s1 %= ADLER_MOD, s2 %= ADLER_MOD;
        buflen -= blocklen;
        blocklen = 5552;
    }
    return (unsigned int)(s2 << 16) + (unsigned int)s1;
}

static unsigned int stb_decompress(unsigned char *output, const unsigned char *i, unsigned int /*length*/)
{
    if (stb__in4(0) != 0x57bC0000) return 0;
    if (stb__in4(4) != 0)          return 0; // error! stream is > 4GB
    const unsigned int olen = stb_decompress_length(i);
    stb__barrier_in_b = i;
    stb__barrier_out_e = output + olen;
    stb__barrier_out_b = output;
    i += 16;

    stb__dout = output;
    for (;;) {
        const unsigned char *old_i = i;
        i = stb_decompress_token(i);
        if (i == old_i) {
            if (*i == 0x05 && i[1] == 0xfa) {
                IM_ASSERT(stb__dout == output + olen);
                if (stb__dout != output + olen) return 0;
                if (stb_adler32(1, output, olen) != (unsigned int) stb__in4(2))
                    return 0;
                return olen;
            } else {
                IM_ASSERT(0); /* NOTREACHED */
                return 0;
            }
        }
        IM_ASSERT(stb__dout <= output + olen);
        if (stb__dout > output + olen)
            return 0;
    }
}

//-----------------------------------------------------------------------------
// [SECTION] Default font data (ProggyClean.ttf)
//-----------------------------------------------------------------------------
// ProggyClean.ttf
// Copyright (c) 2004, 2005 Tristan Grimmer
// MIT license (see License.txt in http://www.upperbounds.net/download/ProggyClean.ttf.zip)
// Download and more information at http://upperbounds.net
//-----------------------------------------------------------------------------
// File: 'ProggyClean.ttf' (41208 bytes)
// Exported using misc/fonts/binary_to_compressed_c.cpp (with compression + base85 string encoding).
// The purpose of encoding as base85 instead of "0x00,0x01,..." style is only save on _source code_ size.
//-----------------------------------------------------------------------------
static const char proggy_clean_ttf_compressed_data_base85[11980 + 1] =
    "7])#######hV0qs'/###[),##/l:$#Q6>##5[n42>c-TH`->>#/e>11NNV=Bv(*:.F?uu#(gRU.o0XGH`$vhLG1hxt9?W`#,5LsCp#-i>.r$<$6pD>Lb';9Crc6tgXmKVeU2cD4Eo3R/"
    "2*>]b(MC;$jPfY.;h^`IWM9<Lh2TlS+f-s$o6Q<BWH`YiU.xfLq$N;$0iR/GX:U(jcW2p/W*q?-qmnUCI;jHSAiFWM.R*kU@C=GH?a9wp8f$e.-4^Qg1)Q-GL(lf(r/7GrRgwV%MS=C#"
    "`8ND>Qo#t'X#(v#Y9w0#1D$CIf;W'#pWUPXOuxXuU(H9M(1<q-UE31#^-V'8IRUo7Qf./L>=Ke$$'5F%)]0^#0X@U.a<r:QLtFsLcL6##lOj)#.Y5<-R&KgLwqJfLgN&;Q?gI^#DY2uL"
    "i@^rMl9t=cWq6##weg>$FBjVQTSDgEKnIS7EM9>ZY9w0#L;>>#Mx&4Mvt//L[MkA#W@lK.N'[0#7RL_&#w+F%HtG9M#XL`N&.,GM4Pg;-<nLENhvx>-VsM.M0rJfLH2eTM`*oJMHRC`N"
    "kfimM2J,W-jXS:)r0wK#@Fge$U>`w'N7G#$#fB#$E^$#:9:hk+eOe--6x)F7*E%?76%^GMHePW-Z5l'&GiF#$956:rS?dA#fiK:)Yr+`&#0j@'DbG&#^$PG.Ll+DNa<XCMKEV*N)LN/N"
    "*b=%Q6pia-Xg8I$<MR&,VdJe$<(7G;Ckl'&hF;;$<_=X(b.RS%%)###MPBuuE1V:v&cX&#2m#(&cV]`k9OhLMbn%s$G2,B$BfD3X*sp5#l,$R#]x_X1xKX%b5U*[r5iMfUo9U`N99hG)"
    "tm+/Us9pG)XPu`<0s-)WTt(gCRxIg(%6sfh=ktMKn3j)<6<b5Sk_/0(^]AaN#(p/L>&VZ>1i%h1S9u5o@YaaW$e+b<TWFn/Z:Oh(Cx2$lNEoN^e)#CFY@@I;BOQ*sRwZtZxRcU7uW6CX"
    "ow0i(?$Q[cjOd[P4d)]>ROPOpxTO7Stwi1::iB1q)C_=dV26J;2,]7op$]uQr@_V7$q^%lQwtuHY]=DX,n3L#0PHDO4f9>dC@O>HBuKPpP*E,N+b3L#lpR/MrTEH.IAQk.a>D[.e;mc."
    "x]Ip.PH^'/aqUO/$1WxLoW0[iLA<QT;5HKD+@qQ'NQ(3_PLhE48R.qAPSwQ0/WK?Z,[x?-J;jQTWA0X@KJ(_Y8N-:/M74:/-ZpKrUss?d#dZq]DAbkU*JqkL+nwX@@47`5>w=4h(9.`G"
    "CRUxHPeR`5Mjol(dUWxZa(>STrPkrJiWx`5U7F#.g*jrohGg`cg:lSTvEY/EV_7H4Q9[Z%cnv;JQYZ5q.l7Zeas:HOIZOB?G<Nald$qs]@]L<J7bR*>gv:[7MI2k).'2($5FNP&EQ(,)"
    "U]W]+fh18.vsai00);D3@4ku5P?DP8aJt+;qUM]=+b'8@;mViBKx0DE[-auGl8:PJ&Dj+M6OC]O^((##]`0i)drT;-7X`=-H3[igUnPG-NZlo.#k@h#=Ork$m>a>$-?Tm$UV(?#P6YY#"
    "'/###xe7q.73rI3*pP/$1>s9)W,JrM7SN]'/4C#v$U`0#V.[0>xQsH$fEmPMgY2u7Kh(G%siIfLSoS+MK2eTM$=5,M8p`A.;_R%#u[K#$x4AG8.kK/HSB==-'Ie/QTtG?-.*^N-4B/ZM"
    "_3YlQC7(p7q)&](`6_c)$/*JL(L-^(]$wIM`dPtOdGA,U3:w2M-0<q-]L_?^)1vw'.,MRsqVr.L;aN&#/EgJ)PBc[-f>+WomX2u7lqM2iEumMTcsF?-aT=Z-97UEnXglEn1K-bnEO`gu"
    "Ft(c%=;Am_Qs@jLooI&NX;]0#j4#F14;gl8-GQpgwhrq8'=l_f-b49'UOqkLu7-##oDY2L(te+Mch&gLYtJ,MEtJfLh'x'M=$CS-ZZ%P]8bZ>#S?YY#%Q&q'3^Fw&?D)UDNrocM3A76/"
    "/oL?#h7gl85[qW/NDOk%16ij;+:1a'iNIdb-ou8.P*w,v5#EI$TWS>Pot-R*H'-SEpA:g)f+O$%%`kA#G=8RMmG1&O`>to8bC]T&$,n.LoO>29sp3dt-52U%VM#q7'DHpg+#Z9%H[K<L"
    "%a2E-grWVM3@2=-k22tL]4$##6We'8UJCKE[d_=%wI;'6X-GsLX4j^SgJ$##R*w,vP3wK#iiW&#*h^D&R?jp7+/u&#(AP##XU8c$fSYW-J95_-Dp[g9wcO&#M-h1OcJlc-*vpw0xUX&#"
    "OQFKNX@QI'IoPp7nb,QU//MQ&ZDkKP)X<WSVL(68uVl&#c'[0#(s1X&xm$Y%B7*K:eDA323j998GXbA#pwMs-jgD$9QISB-A_(aN4xoFM^@C58D0+Q+q3n0#3U1InDjF682-SjMXJK)("
    "h$hxua_K]ul92%'BOU&#BRRh-slg8KDlr:%L71Ka:.A;%YULjDPmL<LYs8i#XwJOYaKPKc1h:'9Ke,g)b),78=I39B;xiY$bgGw-&.Zi9InXDuYa%G*f2Bq7mn9^#p1vv%#(Wi-;/Z5h"
    "o;#2:;%d&#x9v68C5g?ntX0X)pT`;%pB3q7mgGN)3%(P8nTd5L7GeA-GL@+%J3u2:(Yf>et`e;)f#Km8&+DC$I46>#Kr]]u-[=99tts1.qb#q72g1WJO81q+eN'03'eM>&1XxY-caEnO"
    "j%2n8)),?ILR5^.Ibn<-X-Mq7[a82Lq:F&#ce+S9wsCK*x`569E8ew'He]h:sI[2LM$[guka3ZRd6:t%IG:;$%YiJ:Nq=?eAw;/:nnDq0(CYcMpG)qLN4$##&J<j$UpK<Q4a1]MupW^-"
    "sj_$%[HK%'F####QRZJ::Y3EGl4'@%FkiAOg#p[##O`gukTfBHagL<LHw%q&OV0##F=6/:chIm0@eCP8X]:kFI%hl8hgO@RcBhS-@Qb$%+m=hPDLg*%K8ln(wcf3/'DW-$.lR?n[nCH-"
    "eXOONTJlh:.RYF%3'p6sq:UIMA945&^HFS87@$EP2iG<-lCO$%c`uKGD3rC$x0BL8aFn--`ke%#HMP'vh1/R&O_J9'um,.<tx[@%wsJk&bUT2`0uMv7gg#qp/ij.L56'hl;.s5CUrxjO"
    "M7-##.l+Au'A&O:-T72L]P`&=;ctp'XScX*rU.>-XTt,%OVU4)S1+R-#dg0/Nn?Ku1^0f$B*P:Rowwm-`0PKjYDDM'3]d39VZHEl4,.j']Pk-M.h^&:0FACm$maq-&sgw0t7/6(^xtk%"
    "LuH88Fj-ekm>GA#_>568x6(OFRl-IZp`&b,_P'$M<Jnq79VsJW/mWS*PUiq76;]/NM_>hLbxfc$mj`,O;&%W2m`Zh:/)Uetw:aJ%]K9h:TcF]u_-Sj9,VK3M.*'&0D[Ca]J9gp8,kAW]"
    "%(?A%R$f<->Zts'^kn=-^@c4%-pY6qI%J%1IGxfLU9CP8cbPlXv);C=b),<2mOvP8up,UVf3839acAWAW-W?#ao/^#%KYo8fRULNd2.>%m]UK:n%r$'sw]J;5pAoO_#2mO3n,'=H5(et"
    "Hg*`+RLgv>=4U8guD$I%D:W>-r5V*%j*W:Kvej.Lp$<M-SGZ':+Q_k+uvOSLiEo(<aD/K<CCc`'Lx>'?;++O'>()jLR-^u68PHm8ZFWe+ej8h:9r6L*0//c&iH&R8pRbA#Kjm%upV1g:"
    "a_#Ur7FuA#(tRh#.Y5K+@?3<-8m0$PEn;J:rh6?I6uG<-`wMU'ircp0LaE_OtlMb&1#6T.#FDKu#1Lw%u%+GM+X'e?YLfjM[VO0MbuFp7;>Q&#WIo)0@F%q7c#4XAXN-U&VB<HFF*qL("
    "$/V,;(kXZejWO`<[5?\?ewY(*9=%wDc;,u<'9t3W-(H1th3+G]ucQ]kLs7df($/*JL]@*t7Bu_G3_7mp7<iaQjO@.kLg;x3B0lqp7Hf,^Ze7-##@/c58Mo(3;knp0%)A7?-W+eI'o8)b<"
    "nKnw'Ho8C=Y>pqB>0ie&jhZ[?iLR@@_AvA-iQC(=ksRZRVp7`.=+NpBC%rh&3]R:8XDmE5^V8O(x<<aG/1N$#FX$0V5Y6x'aErI3I$7x%E`v<-BY,)%-?Psf*l?%C3.mM(=/M0:JxG'?"
    "7WhH%o'a<-80g0NBxoO(GH<dM]n.+%q@jH?f.UsJ2Ggs&4<-e47&Kl+f//9@`b+?.TeN_&B8Ss?v;^Trk;f#YvJkl&w$]>-+k?'(<S:68tq*WoDfZu';mM?8X[ma8W%*`-=;D.(nc7/;"
    ")g:T1=^J$&BRV(-lTmNB6xqB[@0*o.erM*<SWF]u2=st-*(6v>^](H.aREZSi,#1:[IXaZFOm<-ui#qUq2$##Ri;u75OK#(RtaW-K-F`S+cF]uN`-KMQ%rP/Xri.LRcB##=YL3BgM/3M"
    "D?@f&1'BW-)Ju<L25gl8uhVm1hL$##*8###'A3/LkKW+(^rWX?5W_8g)a(m&K8P>#bmmWCMkk&#TR`C,5d>g)F;t,4:@_l8G/5h4vUd%&%950:VXD'QdWoY-F$BtUwmfe$YqL'8(PWX("
    "P?^@Po3$##`MSs?DWBZ/S>+4%>fX,VWv/w'KD`LP5IbH;rTV>n3cEK8U#bX]l-/V+^lj3;vlMb&[5YQ8#pekX9JP3XUC72L,,?+Ni&co7ApnO*5NK,((W-i:$,kp'UDAO(G0Sq7MVjJs"
    "bIu)'Z,*[>br5fX^:FPAWr-m2KgL<LUN098kTF&#lvo58=/vjDo;.;)Ka*hLR#/k=rKbxuV`>Q_nN6'8uTG&#1T5g)uLv:873UpTLgH+#FgpH'_o1780Ph8KmxQJ8#H72L4@768@Tm&Q"
    "h4CB/5OvmA&,Q&QbUoi$a_%3M01H)4x7I^&KQVgtFnV+;[Pc>[m4k//,]1?#`VY[Jr*3&&slRfLiVZJ:]?=K3Sw=[$=uRB?3xk48@aeg<Z'<$#4H)6,>e0jT6'N#(q%.O=?2S]u*(m<-"
    "V8J'(1)G][68hW$5'q[GC&5j`TE?m'esFGNRM)j,ffZ?-qx8;->g4t*:CIP/[Qap7/9'#(1sao7w-.qNUdkJ)tCF&#B^;xGvn2r9FEPFFFcL@.iFNkTve$m%#QvQS8U@)2Z+3K:AKM5i"
    "sZ88+dKQ)W6>J%CL<KE>`.d*(B`-n8D9oK<Up]c$X$(,)M8Zt7/[rdkqTgl-0cuGMv'?>-XV1q['-5k'cAZ69e;D_?$ZPP&s^+7])$*$#@QYi9,5P&#9r+$%CE=68>K8r0=dSC%%(@p7"
    ".m7jilQ02'0-VWAg<a/''3u.=4L$Y)6k/K:_[3=&jvL<L0C/2'v:^;-DIBW,B4E68:kZ;%?8(Q8BH=kO65BW?xSG&#@uU,DS*,?.+(o(#1vCS8#CHF>TlGW'b)Tq7VT9q^*^$$.:&N@@"
    "$&)WHtPm*5_rO0&e%K&#-30j(E4#'Zb.o/(Tpm$>K'f@[PvFl,hfINTNU6u'0pao7%XUp9]5.>%h`8_=VYbxuel.NTSsJfLacFu3B'lQSu/m6-Oqem8T+oE--$0a/k]uj9EwsG>%veR*"
    "hv^BFpQj:K'#SJ,sB-'#](j.Lg92rTw-*n%@/;39rrJF,l#qV%OrtBeC6/,;qB3ebNW[?,Hqj2L.1NP&GjUR=1D8QaS3Up&@*9wP?+lo7b?@%'k4`p0Z$22%K3+iCZj?XJN4Nm&+YF]u"
    "@-W$U%VEQ/,,>>#)D<h#`)h0:<Q6909ua+&VU%n2:cG3FJ-%@Bj-DgLr`Hw&HAKjKjseK</xKT*)B,N9X3]krc12t'pgTV(Lv-tL[xg_%=M_q7a^x?7Ubd>#%8cY#YZ?=,`Wdxu/ae&#"
    "w6)R89tI#6@s'(6Bf7a&?S=^ZI_kS&ai`&=tE72L_D,;^R)7[$s<Eh#c&)q.MXI%#v9ROa5FZO%sF7q7Nwb&#ptUJ:aqJe$Sl68%.D###EC><?-aF&#RNQv>o8lKN%5/$(vdfq7+ebA#"
    "u1p]ovUKW&Y%q]'>$1@-[xfn$7ZTp7mM,G,Ko7a&Gu%G[RMxJs[0MM%wci.LFDK)(<c`Q8N)jEIF*+?P2a8g%)$q]o2aH8C&<SibC/q,(e:v;-b#6[$NtDZ84Je2KNvB#$P5?tQ3nt(0"
    "d=j.LQf./Ll33+(;q3L-w=8dX$#WF&uIJ@-bfI>%:_i2B5CsR8&9Z&#=mPEnm0f`<&c)QL5uJ#%u%lJj+D-r;BoF&#4DoS97h5g)E#o:&S4weDF,9^Hoe`h*L+_a*NrLW-1pG_&2UdB8"
    "6e%B/:=>)N4xeW.*wft-;$'58-ESqr<b?UI(_%@[P46>#U`'6AQ]m&6/`Z>#S?YY#Vc;r7U2&326d=w&H####?TZ`*4?&.MK?LP8Vxg>$[QXc%QJv92.(Db*B)gb*BM9dM*hJMAo*c&#"
    "b0v=Pjer]$gG&JXDf->'StvU7505l9$AFvgYRI^&<^b68?j#q9QX4SM'RO#&sL1IM.rJfLUAj221]d##DW=m83u5;'bYx,*Sl0hL(W;;$doB&O/TQ:(Z^xBdLjL<Lni;''X.`$#8+1GD"
    ":k$YUWsbn8ogh6rxZ2Z9]%nd+>V#*8U_72Lh+2Q8Cj0i:6hp&$C/:p(HK>T8Y[gHQ4`4)'$Ab(Nof%V'8hL&#<NEdtg(n'=S1A(Q1/I&4([%dM`,Iu'1:_hL>SfD07&6D<fp8dHM7/g+"
    "tlPN9J*rKaPct&?'uBCem^jn%9_K)<,C5K3s=5g&GmJb*[SYq7K;TRLGCsM-$$;S%:Y@r7AK0pprpL<Lrh,q7e/%KWK:50I^+m'vi`3?%Zp+<-d+$L-Sv:@.o19n$s0&39;kn;S%BSq*"
    "$3WoJSCLweV[aZ'MQIjO<7;X-X;&+dMLvu#^UsGEC9WEc[X(wI7#2.(F0jV*eZf<-Qv3J-c+J5AlrB#$p(H68LvEA'q3n0#m,[`*8Ft)FcYgEud]CWfm68,(aLA$@EFTgLXoBq/UPlp7"
    ":d[/;r_ix=:TF`S5H-b<LI&HY(K=h#)]Lk$K14lVfm:x$H<3^Ql<M`$OhapBnkup'D#L$Pb_`N*g]2e;X/Dtg,bsj&K#2[-:iYr'_wgH)NUIR8a1n#S?Yej'h8^58UbZd+^FKD*T@;6A"
    "7aQC[K8d-(v6GI$x:T<&'Gp5Uf>@M.*J:;$-rv29'M]8qMv-tLp,'886iaC=Hb*YJoKJ,(j%K=H`K.v9HggqBIiZu'QvBT.#=)0ukruV&.)3=(^1`o*Pj4<-<aN((^7('#Z0wK#5GX@7"
    "u][`*S^43933A4rl][`*O4CgLEl]v$1Q3AeF37dbXk,.)vj#x'd`;qgbQR%FW,2(?LO=s%Sc68%NP'##Aotl8x=BE#j1UD([3$M(]UI2LX3RpKN@;/#f'f/&_mt&F)XdF<9t4)Qa.*kT"
    "LwQ'(TTB9.xH'>#MJ+gLq9-##@HuZPN0]u:h7.T..G:;$/Usj(T7`Q8tT72LnYl<-qx8;-HV7Q-&Xdx%1a,hC=0u+HlsV>nuIQL-5<N?)NBS)QN*_I,?&)2'IM%L3I)X((e/dl2&8'<M"
    ":^#M*Q+[T.Xri.LYS3v%fF`68h;b-X[/En'CR.q7E)p'/kle2HM,u;^%OKC-N+Ll%F9CF<Nf'^#t2L,;27W:0O@6##U6W7:$rJfLWHj$#)woqBefIZ.PK<b*t7ed;p*_m;4ExK#h@&]>"
    "_>@kXQtMacfD.m-VAb8;IReM3$wf0''hra*so568'Ip&vRs849'MRYSp%:t:h5qSgwpEr$B>Q,;s(C#$)`svQuF$##-D,##,g68@2[T;.XSdN9Qe)rpt._K-#5wF)sP'##p#C0c%-Gb%"
    "hd+<-j'Ai*x&&HMkT]C'OSl##5RG[JXaHN;d'uA#x._U;.`PU@(Z3dt4r152@:v,'R.Sj'w#0<-;kPI)FfJ&#AYJ&#//)>-k=m=*XnK$>=)72L]0I%>.G690a:$##<,);?;72#?x9+d;"
    "^V'9;jY@;)br#q^YQpx:X#Te$Z^'=-=bGhLf:D6&bNwZ9-ZD#n^9HhLMr5G;']d&6'wYmTFmL<LD)F^%[tC'8;+9E#C$g%#5Y>q9wI>P(9mI[>kC-ekLC/R&CH+s'B;K-M6$EB%is00:"
    "+A4[7xks.LrNk0&E)wILYF@2L'0Nb$+pv<(2.768/FrY&h$^3i&@+G%JT'<-,v`3;_)I9M^AE]CN?Cl2AZg+%4iTpT3<n-&%H%b<FDj2M<hH=&Eh<2Len$b*aTX=-8QxN)k11IM1c^j%"
    "9s<L<NFSo)B?+<-(GxsF,^-Eh@$4dXhN$+#rxK8'je'D7k`e;)2pYwPA'_p9&@^18ml1^[@g4t*[JOa*[=Qp7(qJ_oOL^('7fB&Hq-:sf,sNj8xq^>$U4O]GKx'm9)b@p7YsvK3w^YR-"
    "CdQ*:Ir<($u&)#(&?L9Rg3H)4fiEp^iI9O8KnTj,]H?D*r7'M;PwZ9K0E^k&-cpI;.p/6_vwoFMV<->#%Xi.LxVnrU(4&8/P+:hLSKj$#U%]49t'I:rgMi'FL@a:0Y-uA[39',(vbma*"
    "hU%<-SRF`Tt:542R_VV$p@[p8DV[A,?1839FWdF<TddF<9Ah-6&9tWoDlh]&1SpGMq>Ti1O*H&#(AL8[_P%.M>v^-))qOT*F5Cq0`Ye%+$B6i:7@0IX<N+T+0MlMBPQ*Vj>SsD<U4JHY"
    "8kD2)2fU/M#$e.)T4,_=8hLim[&);?UkK'-x?'(:siIfL<$pFM`i<?%W(mGDHM%>iWP,##P`%/L<eXi:@Z9C.7o=@(pXdAO/NLQ8lPl+HPOQa8wD8=^GlPa8TKI1CjhsCTSLJM'/Wl>-"
    "S(qw%sf/@%#B6;/U7K]uZbi^Oc^2n<bhPmUkMw>%t<)'mEVE''n`WnJra$^TKvX5B>;_aSEK',(hwa0:i4G?.Bci.(X[?b*($,=-n<.Q%`(X=?+@Am*Js0&=3bh8K]mL<LoNs'6,'85`"
    "0?t/'_U59@]ddF<#LdF<eWdF<OuN/45rY<-L@&#+fm>69=Lb,OcZV/);TTm8VI;?%OtJ<(b4mq7M6:u?KRdF<gR@2L=FNU-<b[(9c/ML3m;Z[$oF3g)GAWqpARc=<ROu7cL5l;-[A]%/"
    "+fsd;l#SafT/f*W]0=O'$(Tb<[)*@e775R-:Yob%g*>l*:xP?Yb.5)%w_I?7uk5JC+FS(m#i'k.'a0i)9<7b'fs'59hq$*5Uhv##pi^8+hIEBF`nvo`;'l0.^S1<-wUK2/Coh58KKhLj"
    "M=SO*rfO`+qC`W-On.=AJ56>>i2@2LH6A:&5q`?9I3@@'04&p2/LVa*T-4<-i3;M9UvZd+N7>b*eIwg:CC)c<>nO&#<IGe;__.thjZl<%w(Wk2xmp4Q@I#I9,DF]u7-P=.-_:YJ]aS@V"
    "?6*C()dOp7:WL,b&3Rg/.cmM9&r^>$(>.Z-I&J(Q0Hd5Q%7Co-b`-c<N(6r@ip+AurK<m86QIth*#v;-OBqi+L7wDE-Ir8K['m+DDSLwK&/.?-V%U_%3:qKNu$_b*B-kp7NaD'QdWQPK"
    "Yq[@>P)hI;*_F]u`Rb[.j8_Q/<&>uu+VsH$sM9TA%?)(vmJ80),P7E>)tjD%2L=-t#fK[%`v=Q8<FfNkgg^oIbah*#8/Qt$F&:K*-(N/'+1vMB,u()-a.VUU*#[e%gAAO(S>WlA2);Sa"
    ">gXm8YB`1d@K#n]76-a$U,mF<fX]idqd)<3,]J7JmW4`6]uks=4-72L(jEk+:bJ0M^q-8Dm_Z?0olP1C9Sa&H[d&c$ooQUj]Exd*3ZM@-WGW2%s',B-_M%>%Ul:#/'xoFM9QX-$.QN'>"
    "[%$Z$uF6pA6Ki2O5:8w*vP1<-1`[G,)-m#>0`P&#eb#.3i)rtB61(o'$?X3B</R90;eZ]%Ncq;-Tl]#F>2Qft^ae_5tKL9MUe9b*sLEQ95C&`=G?@Mj=wh*'3E>=-<)Gt*Iw)'QG:`@I"
    "wOf7&]1i'S01B+Ev/Nac#9S;=;YQpg_6U`*kVY39xK,[/6Aj7:'1Bm-_1EYfa1+o&o4hp7KN_Q(OlIo@S%;jVdn0'1<Vc52=u`3^o-n1'g4v58Hj&6_t7$##?M)c<$bgQ_'SY((-xkA#"
    "Y(,p'H9rIVY-b,'%bCPF7.J<Up^,(dU1VY*5#WkTU>h19w,WQhLI)3S#f$2(eb,jr*b;3Vw]*7NH%$c4Vs,eD9>XW8?N]o+(*pgC%/72LV-u<Hp,3@e^9UB1J+ak9-TN/mhKPg+AJYd$"
    "MlvAF_jCK*.O-^(63adMT->W%iewS8W6m2rtCpo'RS1R84=@paTKt)>=%&1[)*vp'u+x,VrwN;&]kuO9JDbg=pO$J*.jVe;u'm0dr9l,<*wMK*Oe=g8lV_KEBFkO'oU]^=[-792#ok,)"
    "i]lR8qQ2oA8wcRCZ^7w/Njh;?.stX?Q1>S1q4Bn$)K1<-rGdO'$Wr.Lc.CG)$/*JL4tNR/,SVO3,aUw'DJN:)Ss;wGn9A32ijw%FL+Z0Fn.U9;reSq)bmI32U==5ALuG&#Vf1398/pVo"
    "1*c-(aY168o<`JsSbk-,1N;$>0:OUas(3:8Z972LSfF8eb=c-;>SPw7.6hn3m`9^Xkn(r.qS[0;T%&Qc=+STRxX'q1BNk3&*eu2;&8q$&x>Q#Q7^Tf+6<(d%ZVmj2bDi%.3L2n+4W'$P"
    "iDDG)g,r%+?,$@?uou5tSe2aN_AQU*<h`e-GI7)?OK2A.d7_c)?wQ5AS@DL3r#7fSkgl6-++D:'A,uq7SvlB$pcpH'q3n0#_%dY#xCpr-l<F0NR@-##FEV6NTF6##$l84N1w?AO>'IAO"
    "URQ##V^Fv-XFbGM7Fl(N<3DhLGF%q.1rC$#:T__&Pi68%0xi_&[qFJ(77j_&JWoF.V735&T,[R*:xFR*K5>>#`bW-?4Ne_&6Ne_&6Ne_&n`kr-#GJcM6X;uM6X;uM(.a..^2TkL%oR(#"
    ";u.T%fAr%4tJ8&><1=GHZ_+m9/#H1F^R#SC#*N=BA9(D?v[UiFY>>^8p,KKF.W]L29uLkLlu/+4T<XoIB&hx=T1PcDaB&;HH+-AFr?(m9HZV)FKS8JCw;SD=6[^/DZUL`EUDf]GGlG&>"
    "w$)F./^n3+rlo+DB;5sIYGNk+i1t-69Jg--0pao7Sm#K)pdHW&;LuDNH@H>#/X-TI(;P>#,Gc>#0Su>#4`1?#8lC?#<xU?#@.i?#D:%@#HF7@#LRI@#P_[@#Tkn@#Xw*A#]-=A#a9OA#"
    "d<F&#*;G##.GY##2Sl##6`($#:l:$#>xL$#B.`$#F:r$#JF.%#NR@%#R_R%#Vke%#Zww%#_-4&#3^Rh%Sflr-k'MS.o?.5/sWel/wpEM0%3'/1)K^f1-d>G21&v(35>V`39V7A4=onx4"
    "A1OY5EI0;6Ibgr6M$HS7Q<)58C5w,;WoA*#[%T*#`1g*#d=#+#hI5+#lUG+#pbY+#tnl+#x$),#&1;,#*=M,#.I`,#2Ur,#6b.-#;w[H#iQtA#m^0B#qjBB#uvTB##-hB#'9$C#+E6C#"
    "/QHC#3^ZC#7jmC#;v)D#?,<D#C8ND#GDaD#KPsD#O]/E#g1A5#KA*1#gC17#MGd;#8(02#L-d3#rWM4#Hga1#,<w0#T.j<#O#'2#CYN1#qa^:#_4m3#o@/=#eG8=#t8J5#`+78#4uI-#"
    "m3B2#SB[8#Q0@8#i[*9#iOn8#1Nm;#^sN9#qh<9#:=x-#P;K2#$%X9#bC+.#Rg;<#mN=.#MTF.#RZO.#2?)4#Y#(/#[)1/#b;L/#dAU/#0Sv;#lY$0#n`-0#sf60#(F24#wrH0#%/e0#"
    "TmD<#%JSMFove:CTBEXI:<eh2g)B,3h2^G3i;#d3jD>)4kMYD4lVu`4m`:&5niUA5@(A5BA1]PBB:xlBCC=2CDLXMCEUtiCf&0g2'tN?PGT4CPGT4CPGT4CPGT4CPGT4CPGT4CPGT4CP"
    "GT4CPGT4CPGT4CPGT4CPGT4CPGT4CP-qekC`.9kEg^+F$kwViFJTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5o,^<-28ZI'O?;xp"
    "O?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xp;7q-#lLYI:xvD=#";

static const char* GetDefaultCompressedFontDataTTFBase85()
{
    return proggy_clean_ttf_compressed_data_base85;
}

#endif // #ifndef IMGUI_DISABLE
