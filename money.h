#pragma once

class MONEY;

class DMONEY
{
  friend class MONEY;
private:
  WORD  w0;
  WORD  w1;
  WORD  w2;
  WORD  w3;
  WORD  w4;
  WORD  w5;
  WORD  w6;
  signed short w7;

public:
  DMONEY() {w7 = w6 = w5 = w4 =
        w3 = w2 = w1 = w0 = 0;}
  DMONEY (const MONEY& money);
  DMONEY& operator= (const MONEY& money);

  // some special functions
  void  Negate();
  void  ScaleUp (const MONEY& money);
  MONEY ScaleDown() const;
};

class MONEY : public COleCurrency
{
public:
  // ctors
  MONEY() : COleCurrency() {}
  MONEY(long l) : COleCurrency(l,0) {}
//  MONEY(int i) : COleCurrency(i,0) {}
  MONEY(double d) : COleCurrency(COleVariant(d)) {}
  MONEY(COleCurrency& cur) : COleCurrency(cur) {}
  // binary operators
  MONEY operator+ (const MONEY& money) const { return COleCurrency::operator+(money); }
  MONEY operator- (const MONEY& money) const { return COleCurrency::operator-(money); }
  MONEY operator* (const MONEY& money) const;
  MONEY operator/ (const MONEY& money) const;
  MONEY& operator=  (LONG l) { return *this = COleCurrency(l,0); }
  
  // unary operators
  AFX_INLINE MONEY operator-() const { return COleCurrency::operator -(); }
  // helper operators
  AFX_INLINE MONEY& operator+= (const MONEY& money) { return *this = *this + (COleCurrency)money; }
  AFX_INLINE MONEY& operator-= (const MONEY& money) { return *this = *this - (COleCurrency)money; }
  AFX_INLINE MONEY& operator*= (const MONEY& money) { return *this = *this * money; }
  AFX_INLINE MONEY& operator/= (const MONEY& money) { return *this = *this / money; }
  // conversion operators
  operator double() const
  {
    double d;
    VarR8FromCy(m_cur,&d);
    return d;
  }
  // other helper fns
  AFX_INLINE void Increment() { m_cur.Lo++; }
  MONEY MulDiv (const MONEY& Numerator, const MONEY& Denominator) const;
  MONEY& AddDigit (char digit);
  char RemoveDigit();
  MONEY& Round();
  MONEY& RoundTo (LONG mask = 100L);
  BOOL IsZero() const {return (m_cur.Lo | m_cur.Hi) == 0;}
  BOOL IsNegative() const {return m_cur.Hi < 0;}
  static MONEY MaxNeg() {MONEY v; v.m_cur.Lo = 0x00000000; v.m_cur.Hi = 0x80000000; return v;}
  static MONEY MaxPos() {MONEY v; v.m_cur.Lo = 0xFFFFFFFF; v.m_cur.Hi = 0x7FFFFFFF; return v;}
  void Serialize(CArchive& ar);
};
