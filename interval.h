#ifndef DEF_INTERVAL
#define DEF_INTERVAL

#include   <volume_io.h>

#ifdef  __sgi

/* these five functions are incorrectly prototyped in sys/fpu.h, so
   we correct them here, using defines */

#define  get_fpc_csr    (*get_fpc_csr_ignore)
#define  set_fpc_csr    (*set_fpc_csr_ignore)
#define  get_fpc_irr    (*get_fpc_irr_ignore)
#define  set_fpc_led    (*set_fpc_led_ignore)
#define  get_fpc_eir    (*get_fpc_eir_ignore)

#include   <sys/fpu.h>

extern  int swapRM(int x);

#undef  get_fpc_csr
#undef  set_fpc_csr
#undef  get_fpc_irr
#undef  set_fpc_led
#undef  get_fpc_eir

extern unsigned long get_fpc_csr( void );
extern unsigned long set_fpc_csr( void );
extern unsigned long get_fpc_irr( void );
extern void          set_fpc_led( void );
extern unsigned long get_fpc_eir( void );

#define  SAVE_ROUNDING  int _save
#define  RESTORE_ROUNDING  (void) swapRM( _save )
#define  ROUND_DOWN   swapRM( FUNC_FLOOR )
#define  ROUND_UP     swapRM( FUNC_CEIL )

#else

#define  SAVE_ROUNDING  int _save
#define  RESTORE_ROUNDING  
#define  ROUND_DOWN   
#define  ROUND_UP    

#endif

typedef struct
{
    Real   low, high;
} Interval;

#define  SET_INTERVAL( i, l, h ) \
         { SAVE_ROUNDING;                                                    \
           _save = ROUND_DOWN;                                               \
           (i).low = (l);                                                    \
           (void) ROUND_UP;                                                  \
           (i).high = (h);                                                   \
           RESTORE_ROUNDING;                                                 \
         } 

#define  SET_INTERVAL_MIN( i, m ) \
         { SAVE_ROUNDING;                                                    \
           _save = ROUND_DOWN;                                               \
           (i).low = (m);                                                    \
           RESTORE_ROUNDING;                                                 \
         } 

#define  SET_INTERVAL_MAX( i, m ) \
         { SAVE_ROUNDING;                                                    \
           _save = ROUND_UP;                                                 \
           (i).high = (m);                                                   \
           RESTORE_ROUNDING;                                                 \
         } 

#define  INTERVAL_SIZE( i ) ( (i).high - (i).low )

#define  INTERVAL_MIDPOINT( i ) ( ( (i).low + (i).high ) / 2.0 )

#define  INTERVAL_MIN( i ) ( (i).low )

#define  INTERVAL_MAX( i ) ( (i).high )

#define  INTERVAL_CONTAINS( i, value ) \
     ( (i).low <= (value) && (value) <= (i).high )

#define  INTERVALS_EQUAL( i1, i2 ) \
     ( (i1).low == (i2).low && (i1).high == (i2).high )

#define  ADD_INTERVALS( result, i1, i2 )                                  \
    {                                                                     \
        SAVE_ROUNDING;                                                    \
        _save = ROUND_DOWN;                                               \
        (result).low = (i1).low + (i2).low;                               \
        (void) ROUND_UP;                                                  \
        (result).high = (i1).high + (i2).high;                            \
        RESTORE_ROUNDING;                                                 \
    }

#define  ADD_INTERVAL_REAL( result, i, r )                                \
    {                                                                     \
        SAVE_ROUNDING;                                                    \
        _save = ROUND_DOWN;                                               \
        (result).low = (i).low + (r);                                     \
        (void) ROUND_UP;                                                  \
        (result).high = (i).high + (r);                                   \
        RESTORE_ROUNDING;                                                 \
    }

#define  SUBTRACT_INTERVAL_REAL( result, i, r )                           \
    {                                                                     \
        SAVE_ROUNDING;                                                    \
        _save = ROUND_DOWN;                                               \
        (result).low = (i).low - (r);                                     \
        (void) ROUND_UP;                                                  \
        (result).high = (i).high - (r);                                   \
        RESTORE_ROUNDING;                                                 \
    }

#define  MULT_INTERVAL_REAL( result, i, r )                               \
    {                                                                     \
        Real  al, bl, ah, bh;                                             \
        SAVE_ROUNDING;                                                    \
                                                                          \
        _save = ROUND_DOWN;                                               \
        al = (i).low * (r);                                               \
        bl = (i).high * (r);                                              \
                                                                          \
        (void) ROUND_UP;                                                  \
        ah = (i).low * (r);                                               \
        bh = (i).high * (r);                                              \
        RESTORE_ROUNDING;                                                 \
                                                                          \
        (result).low = MIN( al, bl );                                      \
        (result).high = MAX( ah, bh );                                     \
    }                                                                     \

#define  SUBTRACT_INTERVALS( result, i1, i2 )                             \
    {                                                                     \
        SAVE_ROUNDING;                                                    \
        _save = ROUND_DOWN;                                               \
        (result).low = (i1).low - (i2).high;                              \
        (void) ROUND_UP;                                                  \
        (result).high = (i1).high - (i2).low;                             \
        RESTORE_ROUNDING;                                                 \
    }

#define  MIN4( min, a, b, c, d )                                          \
    {                                                                     \
        Real  min_ab, min_cd;                                             \
                                                                          \
        min_ab = MIN( a, b );                                             \
        min_cd = MIN( c, d );                                             \
        (min) = MIN( min_ab, min_cd );                                    \
    }

#define  MAX4( max, a, b, c, d )                                          \
    {                                                                     \
        Real  max_ab, max_cd;                                             \
                                                                          \
        max_ab = MAX( a, b );                                             \
        max_cd = MAX( c, d );                                             \
        (max) = MAX( max_ab, max_cd );                                    \
    }

#define  MULT_INTERVALS( result, i1, i2 )                                 \
    {                                                                     \
        Real  acl, adl, bcl, bdl;                                         \
        Real  ach, adh, bch, bdh;                                         \
        SAVE_ROUNDING;                                                    \
                                                                          \
        _save = ROUND_DOWN;                                               \
        acl = (i1).low * (i2).low;                                        \
        adl = (i1).low * (i2).high;                                       \
        bcl = (i1).high * (i2).low;                                       \
        bdl = (i1).high * (i2).high;                                      \
                                                                          \
        (void) ROUND_UP;                                                  \
        ach = (i1).low * (i2).low;                                        \
        adh= (i1).low * (i2).high;                                        \
        bch= (i1).high * (i2).low;                                        \
        bdh= (i1).high * (i2).high;                                       \
        RESTORE_ROUNDING;                                                 \
                                                                          \
        MIN4( (result).low, acl, adl, bcl, bdl );                         \
        MAX4( (result).high, ach, adh, bch, bdh );                        \
    }                                                                     \
  
#define  DIVIDE_INTERVAL_REAL( result, i, r )                               \
    {                                                                       \
        Real  al, bl;                                                       \
        Real  ah, bh;                                                       \
                                                                            \
        SAVE_ROUNDING;                                                      \
        _save = ROUND_DOWN;                                                 \
                                                                            \
        al = (i).low / (r);                                                 \
        bl = (i).high / (r);                                                \
                                                                            \
        (void) ROUND_UP;                                                    \
        ah = (i).low / (r);                                                 \
        bh = (i).high / (r);                                                \
        RESTORE_ROUNDING;                                                   \
                                                                            \
        (result).low = MIN( al, bl );                                       \
        (result).high = MAX( ah, bh );                                      \
    }

#define  DIVIDE_INTERVALS( result, i1, i2 )                                 \
    {                                                                       \
        Real  acl, adl, bcl, bdl;                                           \
        Real  ach, adh, bch, bdh;                                           \
        if( (i2).low >= 0.0 || (i2).high <= 0.0 )                           \
        {                                                                   \
          SAVE_ROUNDING;                                                    \
          _save = ROUND_DOWN;                                               \
                                                                            \
          acl = (i1).low / (i2).low;                                        \
          adl = (i1).low / (i2).high;                                       \
          bcl = (i1).high / (i2).low;                                       \
          bdl = (i1).high / (i2).high;                                      \
                                                                            \
          (void) ROUND_UP;                                                  \
          ach = (i1).low / (i2).low;                                        \
          adh = (i1).low / (i2).high;                                       \
          bch = (i1).high / (i2).low;                                       \
          bdh = (i1).high / (i2).high;                                      \
          RESTORE_ROUNDING;                                                 \
                                                                            \
          MIN4( (result).low, acl, adl, bcl, bdl );                         \
          MAX4( (result).high, ach, adh, bch, bdh );                        \
       }                                                                    \
       else                                                                 \
       {                                                                    \
          (result).low = -1.0 / 0.0;                                        \
          (result).high = 1.0 / 0.0;                                        \
       }                                                                    \
    }

#define  SQUARE_INTERVAL( result, i )                                     \
  {                                                                       \
    SAVE_ROUNDING;                                                        \
    _save = ROUND_DOWN;                                                   \
    if( (i).low >= 0.0 )                                                  \
    {                                                                     \
        (result).low = (i).low * (i).low;                                 \
        (void) ROUND_UP;                                                  \
        (result).high = (i).high * (i).high;                              \
    }                                                                     \
    else if( (i).high <= 0.0 )                                            \
    {                                                                     \
        (result).low = (i).high * (i).high;                               \
        (void) ROUND_UP;                                                  \
        (result).high = (i).low * (i).low;                                \
    }                                                                     \
    else                                                                  \
    {                                                                     \
        (result).low = 0.0;                                               \
        (void) ROUND_UP;                                                  \
        if( -(i).low > (i).high )                                         \
            (result).high = (i).low * (i).low;                            \
        else                                                              \
            (result).high = (i).high * (i).high;                          \
    }                                                                     \
    RESTORE_ROUNDING;                                                     \
  }

#define  SQUARE_ROOT_INTERVAL( result, i )                                \
  {                                                                       \
    SAVE_ROUNDING;                                                        \
    _save = ROUND_DOWN;                                                   \
    if( (i).low >= 0.0 )                                                  \
    {                                                                     \
        (result).low = sqrt( (i).low );                                   \
        (void) ROUND_UP;                                                  \
        (result).high = sqrt( (i).high );                                 \
    }                                                                     \
    else if( (i).high <= 0.0 )                                            \
    {                                                                     \
        (result).low = 0.0;                                               \
        (void) ROUND_UP;                                                  \
        (result).high = 0.0;                                              \
    }                                                                     \
    else                                                                  \
    {                                                                     \
        (result).low = 0.0;                                               \
        (void) ROUND_UP;                                                  \
        (result).high = sqrt( (i).high );                                 \
    }                                                                     \
    RESTORE_ROUNDING;                                                     \
  }

#define  COS_INTERVAL( result, i )                                        \
    {                                                                     \
        int     int_a_over_pi;                                            \
        Real    a_over_pi, b_over_pi, cos_low, cos_high;                  \
        SAVE_ROUNDING;                                                    \
        _save = ROUND_DOWN;                                               \
                                                                          \
        a_over_pi = (i).low / PI;                                         \
        b_over_pi = (i).high / PI;                                        \
        int_a_over_pi = CEILING( a_over_pi );                             \
                                                                          \
        if( int_a_over_pi + 1.0 <= b_over_pi )                                \
        {                                                                 \
            (result).low = -1.0;                                          \
            (result).high = 1.0;                                          \
        }                                                                 \
        else                                                              \
        {                                                                 \
            cos_low = cos( (i).low );                                     \
            cos_high = cos( (i).high );                                   \
                                                                          \
            if( int_a_over_pi <= b_over_pi )                                  \
            {                                                             \
                if( int_a_over_pi % 2 == 1 )                                  \
                {                                                         \
                    (result).low = -1.0;                                  \
                    (result).high = MAX( cos_low, cos_high );             \
                }                                                         \
                else                                                      \
                {                                                         \
                    (result).low = MIN( cos_low, cos_high );              \
                    (result).high = 1.0;                                  \
                }                                                         \
            }                                                             \
            else if( cos_low <= cos_high )                                \
            {                                                             \
                (result).low = cos_low;                                   \
                (result).high = cos_high;                                 \
            }                                                             \
            else                                                          \
            {                                                             \
                (result).low = cos_high;                                  \
                (result).high = cos_low;                                  \
            }                                                             \
        }                                                                 \
        RESTORE_ROUNDING;                                                 \
    }

#define  SIN_INTERVAL( result, i )                                        \
    {                                                                     \
        int     int_a_over_pi;                                            \
        Real    a_over_pi, b_over_pi, sin_low, sin_high;                  \
                                                                          \
        SAVE_ROUNDING;                                                    \
        _save = ROUND_DOWN;                                               \
        a_over_pi = (i).low / PI - 0.5;                                   \
        b_over_pi = (i).high / PI - 0.5;                                  \
        int_a_over_pi = CEILING( a_over_pi );                             \
                                                                          \
        if( int_a_over_pi + 1.0 <= b_over_pi )                            \
        {                                                                 \
            (result).low = -1.0;                                          \
            (result).high = 1.0;                                          \
        }                                                                 \
        else                                                              \
        {                                                                 \
            sin_low = sin( (i).low );                                     \
            sin_high = sin( (i).high );                                   \
                                                                          \
            if( int_a_over_pi <= b_over_pi )                                  \
            {                                                             \
                if( int_a_over_pi % 2 == 1 )                                  \
                {                                                         \
                    (result).low = -1.0;                                  \
                    (result).high = MAX( sin_low, sin_high );             \
                }                                                         \
                else                                                      \
                {                                                         \
                    (result).low = MIN( sin_low, sin_high );              \
                    (result).high = 1.0;                                  \
                }                                                         \
            }                                                             \
            else if( sin_low <= sin_high )                                \
            {                                                             \
                (result).low = sin_low;                                   \
                (result).high = sin_high;                                 \
            }                                                             \
            else                                                          \
            {                                                             \
                (result).low = sin_high;                                  \
                (result).high = sin_low;                                  \
            }                                                             \
        }                                                                 \
        RESTORE_ROUNDING;                                                 \
    }

#define  ABS_INTERVAL( result, i )                                        \
  {                                                                       \
    if( (i).low >= 0.0 )                                                  \
    {                                                                     \
        (result) = (i);                                                   \
    }                                                                     \
    else if( (i).high <= 0.0 )                                            \
    {                                                                     \
        (result).low = -(i).high;                                         \
        (result).high = -(i).low;                                         \
    }                                                                     \
    else                                                                  \
    {                                                                     \
        (result).low = 0.0;                                               \
        if( (i).high > -(i).low )                                         \
            (result).high = (i).high;                                     \
        else                                                              \
            (result).high = -(i).low;                                     \
    }                                                                     \
  }

#define  MIN_INTERVALS( result, i1, i2 )                                  \
    {                                                                     \
        (result).low = MIN( (i1).low, (i2).low );                         \
        (result).high = MIN( (i1).high, (i2).high );                      \
    }

#define  MAX_INTERVALS( result, i1, i2 )                                  \
    {                                                                     \
        (result).low = MAX( (i1).low, (i2).low );                         \
        (result).high = MAX( (i1).high, (i2).high );                      \
    }

#define  INTERVAL_SIGN( result, i ) \
    {                                                                      \
        if( (i).high < 0.0 )                                               \
        {                                                                  \
            (result).low = -1.0;                                           \
            (result).high = -1.0;                                          \
        }                                                                  \
        else if( (i).low > 0.0 )                                           \
        {                                                                  \
            (result).low = 1.0;                                            \
            (result).high = 1.0;                                           \
        }                                                                  \
        else if( (i).low < 0.0 && (i).high > 0.0 )                         \
        {                                                                  \
            (result).low = -1.0;                                           \
            (result).high = 1.0;                                           \
        }                                                                  \
        else if( (i).high <= 0.0 && (i).high == 0.0 )                      \
        {                                                                  \
            (result).low = -1.0;                                           \
            (result).high = 0.0;                                           \
        }                                                                  \
        else if( (i).high == 0.0 && (i).low == 0.0 )                        \
        {                                                                  \
            (result).low =  0.0;                                           \
            (result).high = 0.0;                                           \
        }                                                                  \
        else if( (i).high > 0.0 && (i).low == 0.0 )                        \
        {                                                                  \
            (result).low =  0.0;                                           \
            (result).high = 1.0;                                           \
        }                                                                  \
    }

typedef struct
{
    Interval  coord[N_DIMENSIONS];
} IPoint;

typedef  void   (*Interval_function)( int        n_dimensions,
                                      Interval   parameters[],
                                      void       *function_data,
                                      Interval   *value,
                                      Interval   derivatives[] );

#endif
