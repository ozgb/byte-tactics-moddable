# Computer player ratings: numeric details

## Energy rating of a unit type

For each unit type the computer player computes an energy rating, a whole number from 0 to 100:

1. Take the unit's energy cost multiplied by -0.0025, and store it as a 32-bit float.
2. Subtract five times the unit's energy use, as returned by the economy slot `GetEnergyUse` (a negative number for a wind or tidal generator, positive for a consumer). The slot returns the energy use as a 64-bit float (economy 3.0), the exact product of its two 32-bit factors, and the subtraction uses it unrounded. The subtraction is done in double precision, on the stored value of step 1 and that value.
3. If the result is below 0 the rating is 0. If it is above 100 the rating is 100. Otherwise the rating is the result truncated toward zero.

The reference runs at 53-bit precision in test mode.

## What an implementation must do

Steps 1 and 2 are what an implementation must do: the product is stored as a 32-bit float, and the energy use is subtracted at 64 bits, unrounded.

On the shipped game data, two variations of them give the same rating for every unit type and every map:

- keeping the product of step 1 at double precision instead of storing it as a 32-bit float;
- doing that and also rounding the energy use to a 32-bit float.

The checks covered every unit type of the shipped data and every wind speed and tidal strength any shipped map can produce (wind speeds 0 to 8000 in steps of 1, 29 tidal strengths from 0.0 to 48.0), 16,326 combinations. None differs.

The margin is wide. Only ARMWIN and CORWIN (wind generators, 30 energy at full wind) and ARMTIDE and CORTIDE (tidal generators, 1.0 per unit of tidal strength) have a value of step 2 between 0 and 100 that depends on the map. The closest any of them gets to an integer between 1 and 100 is 0.0025 (ARMWIN at wind speed 3309, exact value 97.9975, 0.0025 from 98), more than 400 times the largest rounding error possible there. So on the shipped data the rating does not distinguish these variations. A mod that changes a generator's output, its energy cost, the wind speed range or the tidal strength can bring a value of step 2 within rounding distance of an integer, and there a variation's rating differs from the reference's.

## Other uses of the energy use value

The computer player also tests only the sign of the energy use (is it below 0), for which the rounding is irrelevant.
