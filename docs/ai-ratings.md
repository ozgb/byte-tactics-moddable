# Computer player ratings: numeric details

## Energy rating of a unit type

For each unit type the computer player computes an energy rating, a whole number from 0 to 100:

1. Take the unit's energy cost multiplied by -0.0025, and store it as a 32-bit float.
2. Subtract five times the unit's energy use, as returned by the economy slot `GetEnergyUse` (a negative number for a wind or tidal generator, positive for a consumer). The subtraction is done in double precision, on the stored value of step 1 and the exact energy use.
3. If the result is below 0 the rating is 0. If it is above 100 the rating is 100. Otherwise the rating is the result truncated toward zero.

The reference runs at 53-bit precision in test mode.

## What an implementation may do

An implementation that rounds differently gets the same rating for every unit type and every map in the shipped game data. Two variations were checked:

- keeping the product of step 1 at double precision instead of storing it as a 32-bit float (which is what the modern build's native code does);
- doing that and also rounding the energy use to a 32-bit float, as the result type of `GetEnergyUse` does.

The checks covered every unit type of the shipped data and every wind speed and tidal strength any shipped map can produce (wind speeds 0 to 8000 in steps of 1, 29 tidal strengths from 0.0 to 48.0), 16,326 combinations. None differs.

The margin is wide. Only ARMWIN and CORWIN (wind generators, 30 energy at full wind) and ARMTIDE and CORTIDE (tidal generators, 1.0 per unit of tidal strength) have a value of step 2 between 0 and 100 that depends on the map. The closest any of them gets to an integer between 1 and 100 is 0.0025 (ARMWIN at wind speed 3309, exact value 97.9975, 0.0025 from 98), more than 400 times the largest rounding error possible there. The rating therefore does not distinguish these variations. A mod that changes a generator's output, its energy cost, the wind speed range or the tidal strength can break that margin and should be tested against the reference.

## Other uses of the energy use value

The computer player also tests only the sign of the energy use (is it below 0), for which the rounding is irrelevant.
