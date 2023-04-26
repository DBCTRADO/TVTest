/*
  TVTest
  Copyright(c) 2008-2020 DBCTRADO

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef TVTEST_STYLE_H
#define TVTEST_STYLE_H


#include <unordered_map>


namespace TVTest
{

	namespace Style
	{

		enum class ValueType {
			Void,
			Int,
			Bool,
			String,
		};

		enum class UnitType {
			Undefined,
			LogicalPixel,
			PhysicalPixel,
			Point,
			DIP,
		};

		struct StyleInfo
		{
			String Name;
			ValueType Type = ValueType::Void;
			struct {
				union {
					int Int;
					bool Bool;
				};
				String String;
			} Value;
			UnitType Unit = UnitType::Undefined;
		};

		template<typename T> struct ValueTemplate
		{
			T Value {};
			UnitType Unit = UnitType::Undefined;

			ValueTemplate() = default;
			ValueTemplate(T v, UnitType u = UnitType::LogicalPixel) : Value(v), Unit(u) {}

			operator T() const { return Value; }
			ValueTemplate<T> &operator=(T v) { Value = v; return *this; }

			bool operator==(const ValueTemplate<T> &o) const = default;
			bool operator==(T v) const { return Value == v; }
			bool operator!=(T v) const { return Value != v; }
		};

		typedef ValueTemplate<int> IntValue;

		struct Size
		{
			IntValue Width;
			IntValue Height;

			Size() = default;
			Size(int w, int h, UnitType u = UnitType::LogicalPixel) : Width(w, u), Height(h, u) {}
			Size(int w, UnitType wu, int h, UnitType hu) : Width(w, wu), Height(h, hu) {}

			bool operator==(const Size &o) const noexcept = default;
		};

		struct Margins
		{
			IntValue Left;
			IntValue Top;
			IntValue Right;
			IntValue Bottom;

			Margins() = default;
			Margins(int l, int t, int r, int b, UnitType u = UnitType::LogicalPixel)
				: Left(l, u), Top(t, u), Right(r, u), Bottom(b, u) {}
			Margins(int l, UnitType lu, int t, UnitType tu, int r, UnitType ru, int b, UnitType bu)
				: Left(l, lu), Top(t, tu), Right(r, ru), Bottom(b, bu) {}
			Margins(int m, UnitType u = UnitType::LogicalPixel)
				: Left(m, u), Top(m, u), Right(m, u), Bottom(m, u) {}

			bool operator==(const Margins &o) const noexcept = default;

			int Horz() const { return Left + Right; }
			int Vert() const { return Top + Bottom; }
		};

		struct Font
		{
		public:
			LOGFONT LogFont {};
			IntValue Size;

			Font() = default;
			Font(const LOGFONT &lf, int s, UnitType u = UnitType::Point)
				: LogFont(lf), Size(s, u) {}

			bool operator==(const Font &o) const {
				return CompareLogFont(&LogFont, &o.LogFont) && Size == o.Size;
			}
		};

		class CStyleScaling
		{
		public:
			bool SetDPI(int DPI);
			int GetDPI() const;
			bool SetSystemDPI(int DPI);
			int GetSystemDPI() const;
			void SetScaleFont(bool fScale);
			bool ToPixels(IntValue *pValue) const;
			bool ToPixels(Size *pValue) const;
			bool ToPixels(Margins *pValue) const;
			int ToPixels(int Value, UnitType Unit) const;
			int LogicalPixelsToPhysicalPixels(int Pixels) const;
			int PointsToPixels(int Points) const;
			int DipToPixels(int Dip) const;
			int ConvertUnit(int Value, UnitType SrcUnit, UnitType DstUnit) const;
			bool RealizeFontSize(Font *pFont) const;
			int GetScaledSystemMetrics(int Index, bool fFallbackScaling = true) const;
			bool AdjustWindowRect(HWND hwnd, RECT *pRect) const;

		private:
			int m_DPI = 96;
			int m_SystemDPI = 96;
			bool m_fScaleFont = true;
		};

		class CStyleManager
		{
		public:
			CStyleManager();

			bool Load(LPCTSTR pszFileName);
			bool Set(const StyleInfo &Info);
			bool Get(LPCTSTR pszName, StyleInfo *pInfo) const;
			bool Set(LPCTSTR pszName, const IntValue &Value);
			bool Get(LPCTSTR pszName, IntValue *pValue) const;
			bool Set(LPCTSTR pszName, bool fValue);
			bool Get(LPCTSTR pszName, bool *pfValue) const;
			bool Set(LPCTSTR pszName, const String &Value);
			bool Get(LPCTSTR pszName, String *pValue) const;
			bool Set(LPCTSTR pszName, const Size &Value);
			bool Get(LPCTSTR pszName, Size *pValue) const;
			bool Set(LPCTSTR pszName, const Margins &Value);
			bool Get(LPCTSTR pszName, Margins *pValue) const;
			bool InitStyleScaling(CStyleScaling *pScaling, HMONITOR hMonitor) const;
			bool InitStyleScaling(CStyleScaling *pScaling, HWND hwnd) const;
			bool InitStyleScaling(CStyleScaling *pScaling, const RECT &Position) const;
			bool InitStyleScaling(CStyleScaling *pScaling) const;
			int GetSystemDPI() const;
			int GetForcedDPI() const;
			bool IsHandleDPIChanged() const;
			bool IsUseDarkMenu() const;
			bool IsDarkDialog() const;

			static bool AssignFontSizeFromLogFont(Font *pFont);
			static bool ParseValue(LPCTSTR pszText, IntValue *pValue);

		private:
			typedef std::unordered_map<String, StyleInfo> StyleMap;

			StyleMap m_StyleMap;
			int m_DPI;
			int m_ForcedDPI = 0;
			int m_ResolutionX;
			int m_ResolutionY;
			int m_ForcedResolutionX;
			int m_ForcedResolutionY;
			bool m_fScaleFont = true;
			bool m_fHandleDPIChanged = true;
			bool m_fUseDarkMenu = true;
			bool m_fDarkDialog = true;

			static UnitType ParseUnit(LPCTSTR pszUnit);
		};


		void Add(RECT *pRect, const Margins &margins);
		void Subtract(RECT *pRect, const Margins &margins);
		int GetFontHeight(
			HDC hdc, HFONT hfont, const IntValue &Extra = IntValue(),
			TEXTMETRIC *pTextMetric = nullptr);
		int GetFontHeight(
			HWND hwnd, HFONT hfont, const IntValue &Extra = IntValue(),
			TEXTMETRIC *pTextMetric = nullptr);

	}	// namespace Style

}	// namespace TVTest


#endif	// ndef TVTEST_STYLE_H
