{% import "common_macros.inc.c" as util with context -%}
{{ util.auto_file_banner() }}

{% set defname = "_ICNC_INTERNAL_H_" -%}
#ifndef {{defname}}
#define {{defname}}

extern "C" {
#include "icnc.h"
}

#include <stdint.h>
#include <valarray>

#ifdef DIST_CNC
#include <cnc/dist_cnc.h>
#else
#include <cnc/cnc.h>
#endif /* DIST_CNC */

#define XTRA_PRINTING 0
#include <iostream>

typedef int cncLocation_t;
typedef std::valarray<cncTag_t> cncAggregateTag_t;

template<typename T>
inline std::ostream &cnc_format(std::ostream &os, const std::valarray<T> &t) {
    os << "[ ";
    for (int i=0; i<t.size(); i++) {
        if (i>0) os << ", ";
        os << t[i];
    }
    os << " ]";
    return os;
}

// XXX - All unified CnC collections use an integer tuple tag type
template <> struct cnc_hash<cncAggregateTag_t> {
    size_t operator()(const cncAggregateTag_t &t) const {
        assert(t.size() > 0);
        size_t hash = 0;
        int n = t.size();
        // Knuth's Multiplicative Hash
        for (int i=0; i<n; i++) {
            hash = ((hash << 5) + hash) + t[i]; // hash * 33 + c
        }
#if XTRA_PRINTING
        std::cerr << "cnc#: " << hash << " ";
        cnc_format(std::cerr, t);
        std::cerr << std::endl;
#endif
        return hash;
    }
};

template <> struct cnc_equal<cncAggregateTag_t> {
    bool operator()(const cncAggregateTag_t &tx, const cncAggregateTag_t &ty) const {
        assert(tx.size() == ty.size());
        // Two valarrays are equal if none of the pairwise not-equal comparisons are true
        // We can assume they have the same size.
        bool eq = !(tx != ty).sum();
#if XTRA_PRINTING
        std::cerr << "cnc=: ";
        cnc_format(std::cerr, tx);
        cnc_format(std::cerr, ty);
        std::cerr << " ==> " << eq << std::endl;
#endif
        // XXX - This might be really inefficient. (I should check the assembly.)
        return eq;
    }
};

#ifdef DIST_CNC
namespace CnC {
    template< class T >
    void serialize( CnC::serializer & ser, std::valarray< T > & obj ) {
        const bool unpacking = ser.is_unpacking();
        size_t _sz = 0;
        if (!unpacking) _sz = obj.size();
        ser & _sz;
        if(unpacking) obj.resize(_sz);
        T * _tmp( &obj[0] );
        ser & CnC::chunk< T, CnC::no_alloc >( _tmp, _sz );
        CNC_ASSERT( _tmp == &obj[0] );
#if XTRA_PRINTING
        std::cerr << "tag " << ( ser.is_unpacking() ) << ":";
        cnc_format(std::cerr, obj);
        std::cerr << std::endl;
#endif
    }

    void serialize(CnC::serializer &ser, cncBoxedItem_t *&ptr);
}

#endif /* DIST_CNC */

extern cncAggregateTag_t _cncSingletonTag;

#endif /*{{defname}}*/
