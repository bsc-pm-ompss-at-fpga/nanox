#ifndef CACHEDREGIONSTATUS_DECL_HPP
#define CACHEDREGIONSTATUS_DECL_HPP

#include "version_decl.hpp"
#include "deviceops_decl.hpp"

namespace nanos {

   class CachedRegionStatus : public Version {
      private:
         DeviceOps _ops;
         bool _dirty;
         bool _valid;
      public:
         CachedRegionStatus();
         CachedRegionStatus( CachedRegionStatus const &rs );
         DeviceOps * getDeviceOps();

         CachedRegionStatus &operator=( CachedRegionStatus const &rs );
         CachedRegionStatus( CachedRegionStatus &rs );
         CachedRegionStatus &operator=( CachedRegionStatus &rs );

         bool isDirty() const;
         void setDirty();
         void clearDirty();

         bool isValid() const;
         void setValid( bool flag );

         void resetVersion();
   };
}

#endif /* CACHEDREGIONSTATUS_DECL_HPP */
