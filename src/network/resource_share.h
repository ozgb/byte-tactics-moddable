#ifndef NETWORK_RESOURCE_SHARE_H
#define NETWORK_RESOURCE_SHARE_H

inline float ShareAmount(float stock, float keep, float factor, float capacity, float partnerStock)
{
#ifdef __clang__
    double product = ((double)stock - (double)keep) * (double)factor;
    double limit = (double)capacity - (double)partnerStock;
    return product < limit ? (float)product : (float)limit;
#else
    float amount = (stock - keep) * factor;
    float limit = capacity - partnerStock;
    return amount < limit ? amount : limit;
#endif
}

inline float ShareMetalAmount(float metal, float shareMetal, float capacity, float partnerMetal)
{
    return ShareAmount(metal, shareMetal, 0.33333334f, capacity, partnerMetal);
}

inline float ShareEnergyAmount(float energy, float shareEnergy, float capacity, float partnerEnergy)
{
    return ShareAmount(energy, shareEnergy, 0.5f, capacity, partnerEnergy);
}

#endif
