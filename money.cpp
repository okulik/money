#include "money.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define SFACTOR   10000

inline DMONEY& DMONEY::operator= (const MONEY& money)
{
  w0 = LOWORD(money.m_cur.Lo);
  w1 = HIWORD(money.m_cur.Lo);
  w2 = LOWORD(money.m_cur.Hi);
  w3 = HIWORD(money.m_cur.Hi);
  if (money.IsNegative())
    w7 = w6 = w5 = w4 = 0xFFFF;
  else
    w7 = w6 = w5 = w4 = 0;
  return (*this);
}

inline DMONEY::DMONEY (const MONEY& money)
{
  *this = money;
}

// switch off global optimization, because of inline assembler
#pragma optimize ("lge", off)

inline void DMONEY::Negate()
{
  _asm {
    mov ebx, this
    not [ebx]this.w0
    add [ebx]this.w0,1
    not [ebx]this.w1
    adc [ebx]this.w1,0
    not [ebx]this.w2
    adc [ebx]this.w2,0
    not [ebx]this.w3
    adc [ebx]this.w3,0
    not [ebx]this.w4
    adc [ebx]this.w4,0
    not [ebx]this.w5
    adc [ebx]this.w5,0
    not [ebx]this.w6
    adc [ebx]this.w6,0
    not [ebx]this.w7
    adc [ebx]this.w7,0
  }
}

/*-------------------------------------------------------------------------*/
// multiply money by 10000, and store the result in object
// money must be positive
void DMONEY::ScaleUp (const MONEY& money)
{
  ASSERT(!money.IsNegative());
  WORD w0,w1,w2;
  signed short w3;
  w0 = LOWORD(money.m_cur.Lo);
  w1 = HIWORD(money.m_cur.Lo);
  w2 = LOWORD(money.m_cur.Hi);
  w3 = HIWORD(money.m_cur.Hi);
  _asm {
    mov cx, SFACTOR
    mov ax, w0
    mul cx
    mov ebx, this
    mov [ebx]this.w0, ax
    mov [ebx]this.w1, dx
    mov ax, w1
    mul cx
    add [ebx]this.w1, ax
    adc dx, 0
    mov [ebx]this.w2, dx
    mov ax, w2
    mul cx
    add [ebx]this.w2, ax
    adc dx, 0
    mov [ebx]this.w3, dx
    mov ax, w3
    mul cx
    add [ebx]this.w3, ax
    adc dx, 0
    mov [ebx]this.w4, dx
    mov [ebx]this.w5, 0
    mov [ebx]this.w6, 0
    mov [ebx]this.w7, 0
  }
}

/*-------------------------------------------------------------------------*/
// divide object by 10000 and return a MONEY result,
// the remainder is dropped
// dmoney must be positive
MONEY DMONEY::ScaleDown() const
{
  DMONEY dm = *this;
  // if assert fails, a MONEY overflow is the result
  // can cause a divide by zero
  ASSERT(w7 == 0 && w6 == 0 && w5 == 0);
  MONEY res;
  WORD w0,w1,w2;
  signed short w3;
  _asm {
    mov cx, SFACTOR
    mov dx, [dm].w4
    and dx, 0x1FFF
    mov ax, [dm].w3
    div cx
    mov w3, ax
    mov ax, [dm].w2
    div cx
    mov w2, ax
    mov ax, [dm].w1
    div cx
    mov w1, ax
    mov ax, [dm].w0
    div cx
    mov w0, ax
  }
  res.m_cur.Lo = MAKELONG(w0,w1);
  res.m_cur.Hi = MAKELONG(w2,w3);
  res.m_status = COleCurrency::valid;
  return (w7 < 0) ? -res : res;
}

// MONEY class binary operators

// multiply
MONEY MONEY::operator*(const MONEY& money) const
{
  __int64 i64Multiplicand = money.m_cur.int64,
      i64Multiplier = m_cur.int64,
      i64ProductLo = 0,
      i64ProductHi = 0;
  
  // for unsigned multiplication change all values to positive
  // ****************************************
  // all sign checks are optimized for speed and size
  //   original if statement:
  //     if ( mon.IsNegative() )
  //       mon = -mon;
  //   optimized sign check produces 4 simple assembly instructions

  if ( *((DWORD*)(((BYTE*)&(money.m_cur.int64))+4)) & 0x80000000 )
    i64Multiplicand = -money.m_cur.int64;
  if ( *((DWORD*)(((BYTE*)&(m_cur.int64))+4)) & 0x80000000 )
    i64Multiplier = -m_cur.int64;

  // multiply schema
  // #1 multiply low order DWORDs
  // #2 multiply D * A
  // #3 multiply C times B
  // #4 multiply C * A
  // #5 compute sum of partialproducts
  //
  //    #1           #2         #3          #4           #5     
  //                                                           
  //  A | B        A | B      A | B       A | B        A | B     
  //  -----        -----      -----       -----        -----    
  //  C | D  =>    C | D  =>  C | D  =>   C | D  =>    C | D  
  //  =====        =====      =====       =====        =====    
  //   D*B          D*B        D*B         D*B          D*B     
  //        D*A        D*A         D*A          D*A       
  //                         C*B         C*B          C*B       
  //                                   C*A          C*A
  //                                              + --------
  //                                                  AB*CD
  __asm
  {
    mov eax,dword ptr i64Multiplier
    mov ebx,eax              // save Multiplier value
    mul dword ptr i64Multiplicand    // #1
    mov dword ptr i64ProductLo,eax    // save partial product
    mov ecx,edx              // save high order DWORD
    mov eax,ebx              // get Multiplier from EBX
    mul dword ptr i64Multiplicand+4    // #2
    add eax,ecx              // add partial product (#5)
    adc edx,0              // do not forget carry
    mov ebx,eax              // save partial product...
    mov ecx,edx              //  for now

    mov eax,dword ptr i64Multiplier+4  // get high order Multiplier
    mul dword ptr i64Multiplicand    // #3
    add eax,ebx              // add partial product (#5)
    mov dword ptr i64ProductLo+4,eax  // save partial product
    adc ecx,edx              // add in carry/high order
    mov eax,dword ptr i64Multiplier+4  // high order DWORDs are multiplied
    mul dword ptr i64Multiplicand+4    // #4
    add eax,ecx              // add partial product (#5)
    adc edx,0              // do not forget carry
    mov dword ptr i64ProductHi,eax    // save partial product
    mov dword ptr i64ProductHi+4,edx
    
    // divide result with 10000, ignore modulo
    mov ecx,10000
    mov edx,dword ptr i64ProductHi
    and edx,0x00001FFF
    mov eax,dword ptr i64ProductLo+4
    div ecx
    mov dword ptr i64ProductLo+4,eax
    mov eax,dword ptr i64ProductLo
    div ecx
    mov dword ptr i64ProductLo,eax
  }

  // check for overflow
  ASSERT(!(i64ProductHi & 0xFFFFFFFFFFFF0000));

  MONEY mon;
  mon.m_cur.int64 = i64ProductLo;
  mon.m_status = COleCurrency::valid;

  // if Product is negative, change sign
  if ( *((DWORD*)(((BYTE*)&(mon.m_cur.int64))+4)) & 0x80000000 )
    mon.m_cur.int64 = -mon.m_cur.int64;

  // if Multiplicand and Multiplier have different signs, change Product sign
  if ( (*((DWORD*)(((BYTE*)&(m_cur.int64))+4)) ^ *((DWORD*)(((BYTE*)&(money.m_cur.int64))+4))) & 0x80000000 )
    mon.m_cur.int64 = -mon.m_cur.int64;

  return mon;
}

MONEY MONEY::operator/(const MONEY& money) const
{
  ASSERT(!money.IsZero());

  MONEY quot;
  MONEY dvsr = money.IsNegative() ? -money : money;
  DMONEY dvnd;
  dvnd.ScaleUp (IsNegative() ? -(*this) : *this);

  WORD w0quot,w1quot,w2quot,
     w0dvsr,w1dvsr,w2dvsr;
  signed short w3quot,
         w3dvsr;
  w0dvsr = LOWORD(dvsr.m_cur.Lo);
  w1dvsr = HIWORD(dvsr.m_cur.Lo);
  w2dvsr = LOWORD(dvsr.m_cur.Hi);
  w3dvsr = HIWORD(dvsr.m_cur.Hi);

  _asm {
    mov ecx, 64
nextbit:
    shl w0quot, 1
    rcl w1quot, 1
    rcl w2quot, 1
    rcl w3quot, 1
    shl [dvnd].w0, 1
    rcl [dvnd].w1, 1
    rcl [dvnd].w2, 1
    rcl [dvnd].w3, 1
    rcl [dvnd].w4, 1
    rcl [dvnd].w5, 1
    rcl [dvnd].w6, 1
    rcl [dvnd].w7, 1
    mov dx, 0
    adc dx, 0
    mov ax, w0dvsr
    sub [dvnd].w4, ax
    mov ax, w1dvsr
    sbb [dvnd].w5, ax
    mov ax, w2dvsr
    sbb [dvnd].w6, ax
    mov ax, w3dvsr
    sbb [dvnd].w7, ax
    jnc addbit
    cmp dx, 0
    jz rest
addbit: add w0quot, 1
    loop nextbit
    jmp done
rest:  mov ax, w0dvsr
    add [dvnd].w4, ax
    mov ax, w1dvsr
    adc [dvnd].w5, ax
    mov ax, w2dvsr
    adc [dvnd].w6, ax
    mov ax, w3dvsr
    adc [dvnd].w7, ax
    loop longjump
    jmp done
longjump:
    jmp nextbit
done:
  }
  quot.m_cur.Lo = MAKELONG(w0quot,w1quot);
  quot.m_cur.Hi = MAKELONG(w2quot,w3quot);
  quot.m_status = COleCurrency::valid;
  // drop the remainder
  if (IsNegative() != money.IsNegative())
    return -quot;
  return quot;
}

MONEY MONEY::MulDiv (const MONEY& Numerator, const MONEY& Denominator) const
{
  // check for zero
  if (Denominator.IsZero()) {
    TRACE0("MONEY: Divide by Zero.\r\n");
    return 0L;
  }

  // First Multiply the numbers
  DMONEY prod = Numerator.IsNegative() ? -Numerator : Numerator;
  MONEY mcnd = IsNegative() ? -(*this) : *this;

  WORD w0,w1,w2;
  signed short w3;
  w0 = LOWORD(mcnd.m_cur.Lo);
  w1 = HIWORD(mcnd.m_cur.Lo);
  w2 = LOWORD(mcnd.m_cur.Hi);
  w3 = HIWORD(mcnd.m_cur.Hi);

  _asm {
    mov ecx, 64
nextbit:
    test [prod].w0, 1
    jnz doadd
    clc
    jmp cont
doadd:  mov ax, w0
    add [prod].w4, ax
    mov ax, w1
    adc [prod].w5, ax
    mov ax, w2
    adc [prod].w6, ax
    mov ax, w3
    adc [prod].w7, ax
cont:  rcr [prod].w7,1
    rcr [prod].w6,1
    rcr [prod].w5,1
    rcr [prod].w4,1
    rcr [prod].w3,1
    rcr [prod].w2,1
    rcr [prod].w1,1
    rcr [prod].w0,1
    loop nextbit
  }

  // Now do the division
  MONEY quot;
  MONEY dvsr = Denominator.IsNegative() ? -Denominator : Denominator;
  WORD w0quot,w1quot,w2quot, w0dvsr,w1dvsr,w2dvsr;
  signed short w3quot, w3dvsr;
  w0dvsr = LOWORD(dvsr.m_cur.Lo);
  w1dvsr = HIWORD(dvsr.m_cur.Lo);
  w2dvsr = LOWORD(dvsr.m_cur.Hi);
  w3dvsr = HIWORD(dvsr.m_cur.Hi);

  _asm {
    mov ecx, 64
nextbit2:
    shl w0quot, 1
    rcl w1quot, 1
    rcl w2quot, 1
    rcl w3quot, 1
    shl [prod].w0, 1
    rcl [prod].w1, 1
    rcl [prod].w2, 1
    rcl [prod].w3, 1
    rcl [prod].w4, 1
    rcl [prod].w5, 1
    rcl [prod].w6, 1
    rcl [prod].w7, 1
    mov dx, 0
    adc dx, 0
    mov ax, w0dvsr
    sub [prod].w4, ax
    mov ax, w1dvsr
    sbb [prod].w5, ax
    mov ax, w2dvsr
    sbb [prod].w6, ax
    mov ax, w3dvsr
    sbb [prod].w7, ax
    jnc addbit
    cmp dx, 0
    jz rest
addbit: add w0quot, 1
    loop nextbit2
    jmp done
rest:  mov ax, w0dvsr
    add [prod].w4, ax
    mov ax, w1dvsr
    adc [prod].w5, ax
    mov ax, w2dvsr
    adc [prod].w6, ax
    mov ax, w3dvsr
    adc [prod].w7, ax
    loop longjump
    jmp done
longjump:
    jmp nextbit2
done:
  }

  quot.m_cur.Lo = MAKELONG(w0quot,w1quot);
  quot.m_cur.Hi = MAKELONG(w2quot,w3quot);
  quot.m_status = COleCurrency::valid;
  // drop the remainder
  BOOL isNeg = (IsNegative() != Numerator.IsNegative());
  if (isNeg != Denominator.IsNegative())
    return -quot;
  return quot;
}

MONEY& MONEY::AddDigit (char digit)
{
  ASSERT(digit >= '0' && digit <= '9');

  if( !(digit >= '0' && digit <= '9') )
  {
    m_status = COleCurrency::invalid;
    return (*this);
  }

  //shift left
  m_cur.int64 = m_cur.int64 * 10;
  
  //add the digit
  switch(digit)
  {
    case '1': m_cur.int64 += 1; break;
    case '2': m_cur.int64 += 2; break;
    case '3': m_cur.int64 += 3; break;
    case '4': m_cur.int64 += 4; break;
    case '5': m_cur.int64 += 5; break;
    case '6': m_cur.int64 += 6; break;
    case '7': m_cur.int64 += 7; break;
    case '8': m_cur.int64 += 8; break;
    case '9': m_cur.int64 += 9; break;
  }

  m_status = COleCurrency::valid;
  return (*this);
}

char MONEY::RemoveDigit()
{
  WORD w0,w1,w2;

  signed short w3;
  w0 = LOWORD(m_cur.Lo);
  w1 = HIWORD(m_cur.Lo);
  w2 = LOWORD(m_cur.Hi);
  w3 = HIWORD(m_cur.Hi);
  ASSERT(w3 >= 0);
  char res;
  _asm {
    mov ax, w3
    xor dx, dx
    mov cx, 10
    div cx
    mov w3, ax
    mov ax, w2
    div cx
    mov w2, ax
    mov ax, w1
    div cx
    mov w1, ax
    mov ax, w0
    div cx
    mov w0, ax
    mov res, dl
  }
  m_cur.Lo = MAKELONG(w0,w1);
  m_cur.Hi = (long) MAKELONG(w2,w3);
  return (char)(res + '0');
}

#pragma optimize ("", on)       // global opt. back on

MONEY& MONEY::Round()
{
  if (IsNegative()) *this -= 0.025f;
  else *this += 0.025f;
  *this = *this / (MONEY)500L * (MONEY)500L;
  return *this;
}

/*-------------------------------------------------------------------------*/
// This function rounds the MONEY object to any value from 0.0002 to 200000.00
// 'mask' is the 'floor value' * 10000. ie Fr0.01 is 100L.
MONEY& MONEY::RoundTo (LONG mask)
{
  if (mask > 100L)
  {
    ASSERT(FALSE);
    mask = 100L;
  }
  if (mask > 1) {
    MONEY Operator = (MONEY)mask;
    if (IsNegative()) *this -= Operator / (MONEY)20000L;
    else *this += Operator / (MONEY)20000L;
    *this = (*this / Operator) * Operator;
  }
  return *this;
}

void MONEY::Serialize(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << *this;
  }
  else
  {
    ar >> *this;
  }
}
