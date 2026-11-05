#ifndef UTIL_X87_COMPARE_H
#define UTIL_X87_COMPARE_H

template <class A, class B>
inline bool X87LeTest41(A a, B b)
{
#ifdef __clang__
    return a <= b || __builtin_isunordered(a, b);
#else
    return a <= b;
#endif
}

template <class A, class B>
inline bool X87LtTest01(A a, B b)
{
#ifdef __clang__
    return a < b || __builtin_isunordered(a, b);
#else
    return a < b;
#endif
}

template <class A, class B>
inline bool X87EqTest40(A a, B b)
{
#ifdef __clang__
    return a == b || __builtin_isunordered(a, b);
#else
    return a == b;
#endif
}

inline bool StallsAllowRequest(float energyStall, float metalStall)
{
    return X87LeTest41(energyStall, 0.0f) && X87LeTest41(metalStall, 0.0f);
}

inline bool CloakCostPayable(float cost, float energy)
{
    return X87LeTest41(cost, energy);
}

template <class A, class B>
inline bool SettlementBacklogCovered(A backlog, B avail)
{
    return X87LeTest41(backlog, avail);
}

template <class A, class B>
inline bool SettlementDemandCovered(A demand, B left)
{
    return X87LeTest41(demand, left);
}

inline bool MetalRecipientPoorer(float recipientMetal, float donorMetal)
{
    return X87LtTest01(recipientMetal, donorMetal);
}

inline bool EnergyRecipientPoorer(float recipientEnergy, float donorEnergy)
{
    return X87LtTest01(recipientEnergy, donorEnergy);
}

#endif
