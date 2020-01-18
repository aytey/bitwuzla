/*  Boolector: Satisfiability Modulo Theories (SMT) solver.
 *
 *  Copyright (C) 2018 Mathias Preiner.
 *  Copyright (C) 2018-2019 Aina Niemetz.
 *
 *  This file is part of Boolector.
 *  See COPYING for more information on using this software.
 *
 *  Bit-vector operator invertibility checks based on [1].
 *
 *  [1] Aina Niemetz, Mathias Preiner, Andrew Reynolds, Clark Barrett, Cesare
 *      Tinelli: Solving Quantified Bit-Vectors Using Invertibility Conditions.
 *      CAV (2) 2018: 236-255
 *
 */

#include "bzlainvutils.h"

#include <assert.h>

/* -------------------------------------------------------------------------- */
/* Check invertibility without considering constant bits in x.                */
/* -------------------------------------------------------------------------- */

/**
 * Check invertibility condition (without considering const bits in x) for:
 *
 * x + s = t
 * s + x = t
 *
 * IC: true
 */
bool
bzla_is_inv_add(BzlaMemMgr *mm,
                const BzlaBvDomain *x,
                const BzlaBitVector *t,
                const BzlaBitVector *s,
                uint32_t pos_x)
{
  assert(mm);
  assert(t);
  assert(s);
  (void) mm;
  (void) x;
  (void) t;
  (void) s;
  (void) pos_x;
  return true;
}

/**
 * Check invertibility condition (without considering const bits in x) for:
 *
 * x & s = t
 * s & x = t
 *
 * IC: t & s = t
 */
bool
bzla_is_inv_and(BzlaMemMgr *mm,
                const BzlaBvDomain *x,
                const BzlaBitVector *t,
                const BzlaBitVector *s,
                uint32_t pos_x)
{
  assert(mm);
  assert(t);
  assert(s);
  (void) x;
  (void) pos_x;

  BzlaBitVector *t_and_s = bzla_bv_and(mm, t, s);
  bool res               = bzla_bv_compare(t_and_s, t) == 0;
  bzla_bv_free(mm, t_and_s);
  return res;
}

/**
 * Check invertibility condition (without considering const bits in x) for:
 *
 * pos_x = 0:
 * x o s = t
 * IC: s = t[bw(s) - 1 : 0]
 *
 * pos_x = 1:
 * s o x = t
 * IC: s = t[bw(t) - 1 : bw(t) - bw(s)]
 */
bool
bzla_is_inv_concat(BzlaMemMgr *mm,
                   const BzlaBvDomain *x,
                   const BzlaBitVector *t,
                   const BzlaBitVector *s,
                   uint32_t pos_x)
{
  assert(mm);
  assert(t);
  assert(s);
  (void) x;

  BzlaBitVector *slice;
  bool res;
  uint32_t bw_s, bw_t;

  bw_s = bzla_bv_get_width(s);
  bw_t = bzla_bv_get_width(t);
  if (pos_x == 0)
  {
    slice = bzla_bv_slice(mm, t, bw_s - 1, 0);
  }
  else
  {
    assert(pos_x == 1);
    slice = bzla_bv_slice(mm, t, bw_t - 1, bw_t - bw_s);
  }
  res = bzla_bv_compare(s, slice) == 0;
  bzla_bv_free(mm, slice);
  return res;
}

/**
 * Check invertibility condition (without considering const bits in x) for:
 *
 * x == s = t
 * s == x = t
 *
 * IC: true
 */
bool
bzla_is_inv_eq(BzlaMemMgr *mm,
               const BzlaBvDomain *x,
               const BzlaBitVector *t,
               const BzlaBitVector *s,
               uint32_t pos_x)
{
  assert(mm);
  assert(t);
  assert(s);
  (void) mm;
  (void) x;
  (void) t;
  (void) s;
  (void) pos_x;
  return true;
}

/**
 * Check invertibility condition (without considering const bits in x) for:
 *
 * x * s = t
 * s * x = t
 *
 * IC: (-s | s ) & t = t
 */
bool
bzla_is_inv_mul(BzlaMemMgr *mm,
                const BzlaBvDomain *x,
                const BzlaBitVector *t,
                const BzlaBitVector *s,
                uint32_t pos_x)
{
  assert(mm);
  assert(t);
  assert(s);
  (void) x;
  (void) pos_x;

  BzlaBitVector *neg_s      = bzla_bv_neg(mm, s);
  BzlaBitVector *neg_s_or_s = bzla_bv_or(mm, neg_s, s);
  BzlaBitVector *and_t      = bzla_bv_and(mm, neg_s_or_s, t);
  bool res                  = bzla_bv_compare(and_t, t) == 0;
  bzla_bv_free(mm, neg_s);
  bzla_bv_free(mm, neg_s_or_s);
  bzla_bv_free(mm, and_t);
  return res;
}

/**
 * Check invertibility condition (without considering const bits in x) for:
 *
 * pos_x = 0:
 * x << s = t
 * IC: (t >> s) << s = t
 *
 * pos_x = 1:
 * s << x = t
 * IC: (\/ s << i = t)  i = 0..bw(s)-1
 */
bool
bzla_is_inv_sll(BzlaMemMgr *mm,
                const BzlaBvDomain *x,
                const BzlaBitVector *t,
                const BzlaBitVector *s,
                uint32_t pos_x)
{
  assert(mm);
  assert(t);
  assert(s);
  (void) x;

  bool res;
  if (pos_x == 0)
  {
    BzlaBitVector *t_srl_s = bzla_bv_srl(mm, t, s);
    BzlaBitVector *sll_s   = bzla_bv_sll(mm, t_srl_s, s);
    res                    = bzla_bv_compare(sll_s, t) == 0;
    bzla_bv_free(mm, t_srl_s);
    bzla_bv_free(mm, sll_s);
  }
  else
  {
    assert(pos_x == 1);
    res = false;
    for (uint32_t i = 0, bw_s = bzla_bv_get_width(s); i <= bw_s && !res; i++)
    {
      BzlaBitVector *bv_i    = bzla_bv_uint64_to_bv(mm, i, bw_s);
      BzlaBitVector *s_sll_i = bzla_bv_sll(mm, s, bv_i);
      res                    = bzla_bv_compare(s_sll_i, t) == 0;

      bzla_bv_free(mm, bv_i);
      bzla_bv_free(mm, s_sll_i);
    }
  }
  return res;
}

/**
 * Check invertibility condition (without considering const bits in x) for:
 *
 * pos_x = 0:
 * x >> s = t
 * IC: (t << s) >> s = t
 *
 * pos_x = 1:
 * s >> x = t
 * IC: (\/ s >> i = t)  i = 0..bw(s)-1
 */
bool
bzla_is_inv_srl(BzlaMemMgr *mm,
                const BzlaBvDomain *x,
                const BzlaBitVector *t,
                const BzlaBitVector *s,
                uint32_t pos_x)
{
  assert(mm);
  assert(t);
  assert(s);
  (void) x;

  bool res;
  if (pos_x == 0)
  {
    BzlaBitVector *t_sll_s = bzla_bv_sll(mm, t, s);
    BzlaBitVector *srl_s   = bzla_bv_srl(mm, t_sll_s, s);
    res                    = bzla_bv_compare(srl_s, t) == 0;
    bzla_bv_free(mm, t_sll_s);
    bzla_bv_free(mm, srl_s);
  }
  else
  {
    assert(pos_x == 1);
    res = false;
    for (uint32_t i = 0, bw_s = bzla_bv_get_width(s); i <= bw_s && !res; i++)
    {
      BzlaBitVector *bv_i    = bzla_bv_uint64_to_bv(mm, i, bw_s);
      BzlaBitVector *s_srl_i = bzla_bv_srl(mm, s, bv_i);
      res                    = bzla_bv_compare(s_srl_i, t) == 0;
      bzla_bv_free(mm, bv_i);
      bzla_bv_free(mm, s_srl_i);
    }
  }
  return res;
}

/**
 * Check invertibility condition (without considering const bits in x) for:
 *
 * pos_x = 0:
 * x < s = t
 * IC: t = 0 || s != 0
 *
 * pos_x = 1:
 * s < x = t
 * IC: t = 0 || s != ~0
 */
bool
bzla_is_inv_ult(BzlaMemMgr *mm,
                const BzlaBvDomain *x,
                const BzlaBitVector *t,
                const BzlaBitVector *s,
                uint32_t pos_x)
{
  assert(mm);
  assert(t);
  assert(s);
  (void) mm;
  (void) x;

  bool res;
  if (pos_x == 0)
  {
    res = bzla_bv_is_zero(t) || !bzla_bv_is_zero(s);
  }
  else
  {
    assert(pos_x == 1);
    res = bzla_bv_is_zero(t) || !bzla_bv_is_ones(s);
  }
  return res;
}

/**
 * Check invertibility condition (without considering const bits in x) for:
 *
 * pos_x = 0:
 * x / s = t
 * IC: (s * t) / s = t
 *
 * pos_x = 1:
 * s / x = t
 * IC: s / (s / t) = t
 */
bool
bzla_is_inv_udiv(BzlaMemMgr *mm,
                 const BzlaBvDomain *x,
                 const BzlaBitVector *t,
                 const BzlaBitVector *s,
                 uint32_t pos_x)
{
  assert(mm);
  assert(t);
  assert(s);
  (void) x;

  BzlaBitVector *udiv;
  bool res;
  if (pos_x == 0)
  {
    BzlaBitVector *s_mul_t = bzla_bv_mul(mm, s, t);
    udiv                   = bzla_bv_udiv(mm, s_mul_t, s);
    bzla_bv_free(mm, s_mul_t);
  }
  else
  {
    assert(pos_x == 1);
    BzlaBitVector *s_udiv_t = bzla_bv_udiv(mm, s, t);
    udiv                    = bzla_bv_udiv(mm, s, s_udiv_t);
    bzla_bv_free(mm, s_udiv_t);
  }
  res = bzla_bv_compare(udiv, t) == 0;
  bzla_bv_free(mm, udiv);
  return res;
}

/**
 * Check invertibility condition (without considering const bits in x) for:
 *
 * pos_x = 0:
 * x % s = t
 * IC: ~(-s) >= t
 *
 * pos_x = 1:
 * s % x = t
 * IC: (t + t - s) & s >= t
 */
bool
bzla_is_inv_urem(BzlaMemMgr *mm,
                 const BzlaBvDomain *x,
                 const BzlaBitVector *t,
                 const BzlaBitVector *s,
                 uint32_t pos_x)
{
  assert(mm);
  assert(t);
  assert(s);
  (void) x;

  bool res;
  BzlaBitVector *neg_s = bzla_bv_neg(mm, s);
  if (pos_x == 0)
  {
    BzlaBitVector *not_neg_s = bzla_bv_not(mm, neg_s);
    res                      = bzla_bv_compare(t, not_neg_s) <= 0;
    bzla_bv_free(mm, not_neg_s);
  }
  else
  {
    assert(pos_x == 1);
    BzlaBitVector *t_add_t = bzla_bv_add(mm, t, t);
    BzlaBitVector *sub_s   = bzla_bv_add(mm, t_add_t, neg_s);
    BzlaBitVector *and_s   = bzla_bv_and(mm, sub_s, s);
    res                    = bzla_bv_compare(t, and_s) <= 0;
    bzla_bv_free(mm, t_add_t);
    bzla_bv_free(mm, sub_s);
    bzla_bv_free(mm, and_s);
  }
  bzla_bv_free(mm, neg_s);
  return res;
}

/**
 * Check invertibility condition (without considering const bits in x) for:
 *
 * x[upper:lower] = t
 *
 * IC: true
 */
bool
bzla_is_inv_slice(BzlaMemMgr *mm,
                  const BzlaBvDomain *x,
                  const BzlaBitVector *t,
                  uint32_t upper,
                  uint32_t lower)
{
  assert(mm);
  assert(t);
  (void) mm;
  (void) x;
  (void) t;
  (void) upper;
  (void) lower;
  return true;
}

/* -------------------------------------------------------------------------- */
/* Check invertibility while considering constant bits in x.                  */
/* -------------------------------------------------------------------------- */

/** Check if const bits of domain 'd' match const bits of bit-vector 'bv'. */
static bool
check_const_bits(BzlaMemMgr *mm, const BzlaBvDomain *d, const BzlaBitVector *bv)
{
  bool res;
  BzlaBitVector *and, * or ;
  and = bzla_bv_and(mm, bv, d->hi);
  or  = bzla_bv_or(mm, and, d->lo);
  res = bzla_bv_compare(or, bv) == 0;
  bzla_bv_free(mm, or);
  bzla_bv_free(mm, and);
  return res;
}

/** Check if const bits of domain 'd' match given value 'val'. */
static bool
check_const_bits_val(BzlaMemMgr *mm, const BzlaBvDomain *d, uint32_t val)
{
  bool res;
  uint32_t bw;
  BzlaBitVector *bv;
  bw  = bzla_bv_get_width(d->lo);
  bv  = val ? bzla_bv_ones(mm, bw) : bzla_bv_new(mm, bw);
  res = check_const_bits(mm, d, bv);
  bzla_bv_free(mm, bv);
  return res;
}

/** Check if const bits of domain 'd1' match const bits of domain 'd2'. */
static bool
check_const_domain_bits(BzlaMemMgr *mm,
                        const BzlaBvDomain *d1,
                        const BzlaBvDomain *d2)
{
  bool res;
  BzlaBitVector *const_d1, *const_d2, *common, *masked_d1, *masked_d2;

  const_d1  = bzla_bv_xnor(mm, d1->lo, d1->hi);
  const_d2  = bzla_bv_xnor(mm, d2->lo, d2->hi);
  common    = bzla_bv_and(mm, const_d1, const_d2);
  masked_d1 = bzla_bv_and(mm, common, d1->lo);
  masked_d2 = bzla_bv_and(mm, common, d2->lo);

  res = bzla_bv_compare(masked_d1, masked_d2) == 0;

  bzla_bv_free(mm, const_d1);
  bzla_bv_free(mm, const_d2);
  bzla_bv_free(mm, common);
  bzla_bv_free(mm, masked_d1);
  bzla_bv_free(mm, masked_d2);
  return res;
}

/**
 * Check invertibility condition with respect to const bits in x for:
 *
 * x + s = t
 * s + x = t
 *
 * IC: (((t - s) & hi_x) | lo_x) = t - s
 */
bool
bzla_is_inv_add_const(BzlaMemMgr *mm,
                      const BzlaBvDomain *x,
                      const BzlaBitVector *t,
                      const BzlaBitVector *s,
                      uint32_t pos_x)
{
  assert(mm);
  assert(x);
  assert(t);
  assert(s);
  (void) pos_x;

  bool res;
  BzlaBitVector *sub;

  sub = bzla_bv_sub(mm, t, s);
  res = check_const_bits(mm, x, sub);
  bzla_bv_free(mm, sub);
  return res;
}

/**
 * Check invertibility condition with respect to const bits in x for:
 *
 * x & s = t
 * s & x = t
 *
 * m = ~(lo_x ^ hi_x)  ... mask out all non-constant bits
 * IC: (s & t) = t && (s & hi_x) & m = t & m
 *
 * Intuition:
 * 1) x & s = t on all const bits of x
 * 2) s & t = t on all non-const bits of x
 */
bool
bzla_is_inv_and_const(BzlaMemMgr *mm,
                      const BzlaBvDomain *x,
                      const BzlaBitVector *t,
                      const BzlaBitVector *s,
                      uint32_t pos_x)
{
  assert(mm);
  assert(x);
  assert(t);
  assert(s);

  bool res;
  BzlaBitVector *and1, *and2, *and3, *mask;

  if (!bzla_is_inv_and(mm, x, t, s, pos_x)) return false;

  mask = bzla_bv_xnor(mm, x->lo, x->hi);
  and1 = bzla_bv_and(mm, s, x->hi);
  and2 = bzla_bv_and(mm, and1, mask);
  and3 = bzla_bv_and(mm, t, mask);
  res  = bzla_bv_compare(and2, and3) == 0;
  bzla_bv_free(mm, and1);
  bzla_bv_free(mm, and2);
  bzla_bv_free(mm, and3);
  bzla_bv_free(mm, mask);
  return res;
}

/**
 * Check invertibility condition with respect to const bits in x for:
 *
 * x o s = t
 * IC: (t_h & hi_x) | lo_x = t_h /\ s = t_l
 *     with
 *     t_h = t[bw(t) - 1 : bw(s)]
 *     t_l = t[bw(s) - 1 : 0]
 *
 * s o x = t
 * IC: (t_l & hi_x) | lo_x = t_l /\ s = t_h
 *     with
 *     t_h = t[bw(t) - 1 : bw(x)]
 *     t_l = t[bw(x) - 1 : 0]
 */
bool
bzla_is_inv_concat_const(BzlaMemMgr *mm,
                         const BzlaBvDomain *x,
                         const BzlaBitVector *t,
                         const BzlaBitVector *s,
                         uint32_t pos_x)
{
  assert(mm);
  assert(x);
  assert(t);
  assert(s);

  bool res;
  uint32_t bw_t, bw_s, bw_x;
  BzlaBitVector *t_h, *t_l, *t_and, *t_s, *and, * or ;

  bw_t = bzla_bv_get_width(t);
  bw_s = bzla_bv_get_width(s);
  bw_x = bzla_bvprop_get_width(x);

  if (pos_x == 0)
  {
    t_h   = bzla_bv_slice(mm, t, bw_t - 1, bw_s);
    t_l   = bzla_bv_slice(mm, t, bw_s - 1, 0);
    t_and = t_h;
    t_s   = t_l;
  }
  else
  {
    t_h   = bzla_bv_slice(mm, t, bw_t - 1, bw_x);
    t_l   = bzla_bv_slice(mm, t, bw_x - 1, 0);
    t_and = t_l;
    t_s   = t_h;
  }

  and = bzla_bv_and(mm, t_and, x->hi);
  or  = bzla_bv_or(mm, and, x->lo);
  res = bzla_bv_compare(or, t_and) == 0 && bzla_bv_compare(s, t_s) == 0;
  bzla_bv_free(mm, t_h);
  bzla_bv_free(mm, t_l);
  bzla_bv_free(mm, or);
  bzla_bv_free(mm, and);
  return res;
}

/**
 * Check invertibility condition with respect to const bits in x for:
 *
 * x == s = t
 * s == x = t
 *
 * IC:
 * t = 0: (hi_x != lo_x) || (hi_x != s)
 * t = 1: ((s & hi_x) | lo_x) = s
 */
bool
bzla_is_inv_eq_const(BzlaMemMgr *mm,
                     const BzlaBvDomain *x,
                     const BzlaBitVector *t,
                     const BzlaBitVector *s,
                     uint32_t pos_x)
{
  assert(mm);
  assert(x);
  assert(t);
  assert(s);
  (void) pos_x;

  if (bzla_bv_is_false(t))
  {
    return bzla_bv_compare(x->hi, x->lo) || bzla_bv_compare(x->hi, s);
  }

  return check_const_bits(mm, x, s);
}

bool
bzla_is_inv_mul_const(BzlaMemMgr *mm,
                      const BzlaBvDomain *x,
                      const BzlaBitVector *t,
                      const BzlaBitVector *s,
                      uint32_t pos_x)
{
  assert(mm);
  assert(x);
  assert(t);
  assert(s);
  (void) pos_x;

  bool res = true, lsb_s;
  BzlaBitVector *mod_inv_s, *tmp;

  res = bzla_is_inv_mul(mm, x, t, s, pos_x);

  if (res && !bzla_bv_is_zero(s) && bzla_bvprop_has_fixed_bits(mm, x))
  {
    /* x is constant */
    if (bzla_bvprop_is_fixed(mm, x))
    {
      tmp = bzla_bv_mul(mm, x->lo, s);
      res = bzla_bv_compare(tmp, t) == 0;
      bzla_bv_free(mm, tmp);
    }
    else
    {
      lsb_s = bzla_bv_get_bit(s, 0) == 1;

      /* s odd */
      if (lsb_s)
      {
        mod_inv_s = bzla_bv_mod_inverse(mm, s);
        tmp       = bzla_bv_mul(mm, mod_inv_s, t);
        res       = check_const_bits(mm, x, tmp);
        bzla_bv_free(mm, tmp);
        bzla_bv_free(mm, mod_inv_s);
      } /* s even */
      else
      {
        /* x = (t >> ctz(s)) * (s >> ctz(s))^-1 */

        BzlaBitVector *tmp_s, *tmp_t, *tmp_x, *mask_lo, *mask_hi, *ones;
        BzlaBitVector *lo, *hi;
        BzlaBvDomain *d_tmp_x;
        uint32_t tz_s = bzla_bv_get_num_trailing_zeros(s);
        assert(tz_s <= bzla_bv_get_num_trailing_zeros(t));

        tmp_s = bzla_bv_srl_uint64(mm, s, tz_s);
        tmp_t = bzla_bv_srl_uint64(mm, t, tz_s);

        assert(bzla_bv_get_bit(tmp_s, 0) == 1);

        mod_inv_s = bzla_bv_mod_inverse(mm, tmp_s);
        bzla_bv_free(mm, tmp_s);

        tmp_x = bzla_bv_mul(mm, mod_inv_s, tmp_t);
        bzla_bv_free(mm, tmp_t);
        bzla_bv_free(mm, mod_inv_s);

        /* create domain of x with the most ctz(s) bits set to 'x'. */
        ones    = bzla_bv_ones(mm, bzla_bv_get_width(tmp_x));
        mask_lo = bzla_bv_srl_uint64(mm, ones, tz_s);
        mask_hi = bzla_bv_not(mm, mask_lo);
        bzla_bv_free(mm, ones);

        lo      = bzla_bv_and(mm, mask_lo, tmp_x);
        hi      = bzla_bv_or(mm, mask_hi, tmp_x);
        d_tmp_x = bzla_bvprop_new(mm, lo, hi);
        bzla_bv_free(mm, tmp_x);
        bzla_bv_free(mm, mask_lo);
        bzla_bv_free(mm, mask_hi);
        bzla_bv_free(mm, lo);
        bzla_bv_free(mm, hi);

        res = check_const_domain_bits(mm, d_tmp_x, x);
        bzla_bvprop_free(mm, d_tmp_x);
      }
    }
  }
  return res;
}

/**
 * Check invertibility condition with respect to const bits in x for:
 *
 * pos_x = 0:
 * x << s = t
 * IC: (t >> s) << s = t
 *     /\ (hi_x << s) & t = t
 *     /\ (lo_x << s) | t = t
 *
 * pos_x = 1:
 * s << x = t
 * IC: (\/ s << i = t)  i = 0..bw(s)-1 for all possible i given x
 */
bool
bzla_is_inv_sll_const(BzlaMemMgr *mm,
                      const BzlaBvDomain *x,
                      const BzlaBitVector *t,
                      const BzlaBitVector *s,
                      uint32_t pos_x)
{
  assert(mm);
  assert(x);
  assert(t);
  assert(s);

  bool res, invalid;
  uint32_t bw_s;
  BzlaBitVector *shift1, *shift2, *and, * or ;
  BzlaBitVector *bv_i, *bv_bw;

  if (pos_x == 0)
  {
    if (!bzla_is_inv_sll(mm, x, t, s, pos_x)) return false;
    shift1 = bzla_bv_sll(mm, x->hi, s);
    shift2 = bzla_bv_sll(mm, x->lo, s);
    and    = bzla_bv_and(mm, shift1, t);
    or     = bzla_bv_or(mm, shift2, t);
    res    = bzla_bv_compare(and, t) == 0 && bzla_bv_compare(or, t) == 0;
    bzla_bv_free(mm, or);
    bzla_bv_free(mm, and);
    bzla_bv_free(mm, shift2);
    bzla_bv_free(mm, shift1);
  }
  else
  {
    assert(pos_x == 1);
    bw_s  = bzla_bv_get_width(s);
    bv_bw = bzla_bv_uint64_to_bv(mm, bw_s, bw_s);
    res   = bzla_bv_compare(x->hi, bv_bw) >= 0 && bzla_bv_is_zero(t);
    bzla_bv_free(mm, bv_bw);
    for (uint32_t i = 0; i <= bw_s && !res; i++)
    {
      bv_i = bzla_bv_uint64_to_bv(mm, i, bw_s);

      /* check if bv_i is a possible value given x */
      and = bzla_bv_and(mm, bv_i, x->hi);
      or  = bzla_bv_or(mm, bv_i, x->lo);
      invalid =
          bzla_bv_compare(or, bv_i) != 0 || bzla_bv_compare(and, bv_i) != 0;
      bzla_bv_free(mm, or);
      bzla_bv_free(mm, and);
      if (!invalid)
      {
        /* add to IC */
        shift1 = bzla_bv_sll(mm, s, bv_i);
        res    = bzla_bv_compare(shift1, t) == 0;
        bzla_bv_free(mm, shift1);
      }
      bzla_bv_free(mm, bv_i);
    }
  }
  return res;
}

/**
 * Check invertibility condition with respect to const bits in x for:
 *
 * pos_x = 0:
 * x << s = t
 * IC: (t << s) >> s = t
 *     /\ (hi_x >> s) & t = t
 *     /\ (lo_x >> s) | t = t
 *
 * pos_x = 1:
 * s >> x = t
 * IC: (\/ s >> i = t)  i = 0..bw(s)-1 for all possible i given x
 */
bool
bzla_is_inv_srl_const(BzlaMemMgr *mm,
                      const BzlaBvDomain *x,
                      const BzlaBitVector *t,
                      const BzlaBitVector *s,
                      uint32_t pos_x)
{
  assert(mm);
  assert(x);
  assert(t);
  assert(s);

  bool res, invalid;
  uint32_t bw_s;
  BzlaBitVector *shift1, *shift2, *and, * or ;
  BzlaBitVector *bv_i, *bv_bw;

  if (pos_x == 0)
  {
    if (!bzla_is_inv_srl(mm, x, t, s, pos_x)) return false;
    shift1 = bzla_bv_srl(mm, x->hi, s);
    shift2 = bzla_bv_srl(mm, x->lo, s);
    and    = bzla_bv_and(mm, shift1, t);
    or     = bzla_bv_or(mm, shift2, t);
    res    = bzla_bv_compare(and, t) == 0 && bzla_bv_compare(or, t) == 0;
    bzla_bv_free(mm, or);
    bzla_bv_free(mm, and);
    bzla_bv_free(mm, shift2);
    bzla_bv_free(mm, shift1);
  }
  else
  {
    assert(pos_x == 1);
    bw_s  = bzla_bv_get_width(s);
    bv_bw = bzla_bv_uint64_to_bv(mm, bw_s, bw_s);
    res   = bzla_bv_compare(x->hi, bv_bw) >= 0 && bzla_bv_is_zero(t);
    bzla_bv_free(mm, bv_bw);
    for (uint32_t i = 0; i <= bw_s && !res; i++)
    {
      bv_i = bzla_bv_uint64_to_bv(mm, i, bw_s);

      /* check if bv_i is a possible value given x */
      and = bzla_bv_and(mm, bv_i, x->hi);
      or  = bzla_bv_or(mm, bv_i, x->lo);
      invalid =
          bzla_bv_compare(or, bv_i) != 0 || bzla_bv_compare(and, bv_i) != 0;
      bzla_bv_free(mm, or);
      bzla_bv_free(mm, and);
      if (!invalid)
      {
        /* add to IC */
        shift1 = bzla_bv_srl(mm, s, bv_i);
        res    = bzla_bv_compare(shift1, t) == 0;
        bzla_bv_free(mm, shift1);
      }
      bzla_bv_free(mm, bv_i);
    }
  }
  return res;
}

bool
bzla_is_inv_udiv_const(BzlaMemMgr *mm,
                       const BzlaBvDomain *x,
                       const BzlaBitVector *t,
                       const BzlaBitVector *s,
                       uint32_t pos_x)
{
  assert(mm);
  assert(x);
  assert(t);
  assert(s);
  (void) pos_x;
  return true;
}

/**
 * Check invertibility condition with respect to const bits in x for:
 *
 * x < s = t
 * s < x = t
 *
 * IC pos_x = 0:
 * t = 1: t -> (s != 0 && lo_x < s)
 * t = 0: ~t -> (hi_x >= s)
 *
 *
 * IC pos_x = 1:
 * t = 1: t -> (s != ~0 && hi_x > s)
 * t = 0: ~t -> (lo_x <= s)
 */
bool
bzla_is_inv_ult_const(BzlaMemMgr *mm,
                      const BzlaBvDomain *x,
                      const BzlaBitVector *t,
                      const BzlaBitVector *s,
                      uint32_t pos_x)
{
  assert(mm);
  assert(x);
  assert(t);
  assert(s);

  if (pos_x == 0)
  {
    /* x < s */
    if (bzla_bv_is_true(t))
    {
      return !bzla_bv_is_zero(s) && bzla_bv_compare(x->lo, s) == -1;
    }
    /* x >= s */
    return bzla_bv_compare(x->hi, s) >= 0;
  }
  assert(pos_x == 1);

  /* s < x */
  if (bzla_bv_is_true(t))
  {
    return !bzla_bv_is_ones(s) && bzla_bv_compare(x->hi, s) == 1;
  }
  /* s >= x */
  return bzla_bv_compare(x->lo, s) <= 0;
}

bool
bzla_is_inv_urem_const(BzlaMemMgr *mm,
                       const BzlaBvDomain *x,
                       const BzlaBitVector *t,
                       const BzlaBitVector *s,
                       uint32_t pos_x)
{
  assert(mm);
  assert(x);
  assert(t);
  assert(s);

  bool res;

  res = bzla_is_inv_urem(mm, x, t, s, pos_x);

  if (res)
  {
    BzlaBitVectorPtrStack candidates;
    uint32_t bw;
    int32_t cmp;
    BzlaBitVector *ones, *lo, *hi, *sub, *rem, *div, *bv;

    bw   = bzla_bv_get_width(t);
    ones = bzla_bv_ones(mm, bw);

    if (pos_x)
    {
      if (bzla_bv_compare(t, ones) == 0)
      {
        /* s % x = t = ones: s = ones, x = 0 */
        assert(bzla_bv_compare(s, ones) == 0);
        res = check_const_bits_val(mm, x, 0);
      }
      else
      {
        cmp = bzla_bv_compare(s, t);
        assert(cmp >= 0);
        if (cmp == 0)
        {
          /* s = t and t != ones: x = 0 or random x > t */
          res = bzla_bv_compare(x->hi, t) >= 0;
        }
        else
        {
          /**
           * s > t:
           *
           * x > t, x = (s - t) / n
           * -> (s - t) / n > t and
           * -> (s - t) / t > n
           * -> 1 <= n < (s - t) / t
           *
           * bv division is truncating, thus:
           *
           *    1 <= n <= hi
           *
           * with: t = 0          : hi = s
           *       (s - t) % t = 0: hi = (s - t) / t - 1
           *       (s - t) % t > 0: hi = (s - t) / t
           */
          lo = bzla_bv_one(mm, bw);
          if (bzla_bv_is_zero(t))
          {
            hi = bzla_bv_copy(mm, s);
          }
          else
          {
            sub = bzla_bv_sub(mm, s, t);
            rem = bzla_bv_urem(mm, sub, t);
            div = bzla_bv_udiv(mm, sub, t);
            if (bzla_bv_is_zero(rem))
            {
              hi = bzla_bv_dec(mm, div);
              bzla_bv_free(mm, div);
            }
            else
            {
              hi = div;
            }
            bzla_bv_free(mm, rem);
            bzla_bv_free(mm, sub);
          }

          BZLA_INIT_STACK(mm, candidates);
          BzlaBvDomainGenerator gen;
          bzla_bvprop_gen_init_range(mm, &gen, (BzlaBvDomain *) x, lo, hi);
          while (bzla_bvprop_gen_has_next(&gen))
          {
            bv  = bzla_bvprop_gen_next(&gen);
            rem = bzla_bv_urem(mm, s, bv);
            if (bzla_bv_compare(rem, t) == 0)
            {
              if (check_const_bits(mm, x, rem))
              {
                BZLA_PUSH_STACK(candidates, rem);
              }
              else
              {
                bzla_bv_free(mm, rem);
              }
            }
            else
            {
              bzla_bv_free(mm, rem);
            }
          }
          res = BZLA_EMPTY_STACK(candidates);
          BZLA_RELEASE_STACK(candidates);
        }
      }
    }
    else
    {
      if (bzla_bv_is_zero(s) || bzla_bv_compare(t, ones) == 0)
      {
        /* x % 0 = t: x = t
         * t = ones : s = 0, x = ones */
        res = check_const_bits(mm, x, t);
      }
      else
      {
        assert(bzla_bv_compare(s, t) > 0);
        if (!check_const_bits(mm, x, t))
        {
          /* simplest solution (0 <= res < s: res = t) does not apply, thus
           * x = s * n + t with n s.t. (s * n + t) does not overflow */

          sub = bzla_bv_sub(mm, ones, s);
          if (bzla_bv_compare(sub, t) < 0)
          {
            /* overflow for n = 1 -> only simplest solution possible, but
             * simplest possible solution not applicable */
            res = false;
          }
          else
          {
            /* x = s * n + t, with n s.t. (s * n + t) does not overflow */
          }
        }
      }
    }
  }
  return res;
}

/**
 * Check invertibility condition with respect to const bits in x for:
 *
 * x[upper:lower] = t
 *
 * IC:
 * m = ~(lo_x ^ hi_x)[upper:lower]  ... mask out all non-constant bits
 * x[upper:lower] & m = t & m
 */
bool
bzla_is_inv_slice_const(BzlaMemMgr *mm,
                        const BzlaBvDomain *x,
                        const BzlaBitVector *t,
                        uint32_t upper,
                        uint32_t lower)
{
  assert(mm);
  assert(x);
  assert(t);

  bool res;
  BzlaBitVector *mask, *mask_sliced, *x_mask, *t_mask;

  mask        = bzla_bv_xnor(mm, x->lo, x->hi);
  mask_sliced = bzla_bv_slice(mm, mask, upper, lower);

  x_mask = bzla_bv_slice(mm, x->lo, upper, lower);
  t_mask = bzla_bv_and(mm, mask_sliced, t);
  res    = bzla_bv_compare(x_mask, t_mask) == 0;

  bzla_bv_free(mm, mask);
  bzla_bv_free(mm, mask_sliced);
  bzla_bv_free(mm, x_mask);
  bzla_bv_free(mm, t_mask);

  return res;
}
