#ifndef TVTEST_STYLE_H
#define TVTEST_STYLE_H


#include <unordered_map>


namespace TVTest
{

	namespace Style
	{

		enum ValueType {
			TYPE_VOID,
			TYPE_INT,
			TYPE_BOOL,
			TYPE_STRING
		};

		enum UnitType {
			UNIT_UNDEFINED,
			UNIT_LOGICAL_PIXEL,
			UNIT_PHYSICAL_PIXEL,
			UNIT_POINT,
			UNIT_DIP
		};

		struct StyleInfo
		{
			String Name;
			ValueType Type;
			struct {
				union {
					int Int;
					bool Bool;
				};
				String String;
			} Value;
			UnitType Unit;

			StyleInfo() : Type(TYPE_VOID), Unit(UNIT_UNDEFINED) {}
		};

		template<typename T> struct ValueTemplate
		{
			T Value;
			UnitType Unit;

			ValueTemplate() : Value(0), Unit(UNIT_UNDEFINED) {}
			ValueTemplate(T v,UnitType u=UNIT_LOGICAL_PIXEL) : Value(v), Unit(u) {}
			operator T() const { return Value; }
			ValueTemplate<T> &operator=(T v) { Value=v; return *this; }
			bool operator==(const ValueTemplate<T> &o) const { return Value==o.Value && Unit==o.Unit; }
			bool operator!=(const ValueTemplate<T> &o) const { return !(*this==o); }
		};

		typedef ValueTemplate<int> IntValue;

		struct Size
		{
			IntValue Width;
			IntValue Height;

			Size() {}
			Size(int w,int h,UnitType u=UNIT_LOGICAL_PIXEL) : Width(w,u), Height(h,u) {}
			Size(int w,UnitType wu,int h,UnitType hu) : Width(w,wu), Height(h,hu) {}
			bool operator==(const Size &o) const { return Width==o.Width && Height==o.Height; }
			bool operator!=(const Size &o) const { return !(*this==o); }
		};

		struct Margins
		{
			IntValue Left;
			IntValue Top;
			IntValue Right;
			IntValue Bottom;

			Margins() {}
			Margins(int l,int t,int r,int b,UnitType u=UNIT_LOGICAL_PIXEL)
				: Left(l,u), Top(t,u), Right(r,u), Bottom(b,u) {}
			Margins(int l,UnitType lu,int t,UnitType tu,int r,UnitType ru,int b,UnitType bu)
				: Left(l,lu), Top(t,tu), Right(r,ru), Bottom(b,bu) {}
			Margins(int m,UnitType u=UNIT_LOGICAL_PIXEL)
				: Left(m,u), Top(m,u), Right(m,u), Bottom(m,u) {}
			bool operator==(const Margins &o) const {
				return Left==o.Left && Top==o.Top && Right==o.Right && Bottom==o.Bottom; }
			bool operator!=(const Margins &o) const { return !(*this==o); }
			int Horz() const { return Left+Right; }
			int Vert() const { return Top+Bottom; }
		};

		class CStyleManager
		{
		public:
			CStyleManager();
			bool Load(LPCTSTR pszFileName);
			bool Set(const StyleInfo &Info);
			bool Get(LPCTSTR pszName,StyleInfo *pInfo) const;
			bool Set(LPCTSTR pszName,const IntValue &Value);
			bool Get(LPCTSTR pszName,IntValue *pValue) const;
			bool Set(LPCTSTR pszName,bool fValue);
			bool Get(LPCTSTR pszName,bool *pfValue) const;
			bool Set(LPCTSTR pszName,const String &Value);
			bool Get(LPCTSTR pszName,String *pValue) const;
			bool Set(LPCTSTR pszName,const Size &Value);
			bool Get(LPCTSTR pszName,Size *pValue) const;
			bool Set(LPCTSTR pszName,const Margins &Value);
			bool Get(LPCTSTR pszName,Margins *pValue) const;
			bool ToPixels(IntValue *pValue) const;
			bool ToPixels(Size *pValue) const;
			bool ToPixels(Margins *pValue) const;
			int ToPixels(int Value,UnitType Unit) const;
			int LogicalPixelsToPhysicalPixels(int Pixels) const;
			int PointsToPixels(int Points) const;
			int DipToPixels(int Dip) const;
			int ConvertUnit(int Value,UnitType SrcUnit,UnitType DstUnit) const;
			int GetDPI() const;

		private:
			typedef std::unordered_map<String,StyleInfo> StyleMap;

			StyleMap m_StyleMap;
			int m_ResolutionX;
			int m_ResolutionY;

			static UnitType ParseUnit(LPCTSTR pszUnit);
		};


		void Add(RECT *pRect,const Margins &margins);
		void Subtract(RECT *pRect,const Margins &margins);
		int GetFontHeight(HDC hdc,HFONT hfont,const IntValue &Extra=IntValue(),
						  TEXTMETRIC *pTextMetric=nullptr);
		int GetFontHeight(HWND hwnd,HFONT hfont,const IntValue &Extra=IntValue(),
						  TEXTMETRIC *pTextMetric=nullptr);

	}	// namespace Style

}	// namespace TVTest


#endif	// ndef TVTEST_STYLE_H
