#ifndef NANOS_RESILIENCE_DECL_H
#define NANOS_RESILIENCE_DECL_H

#include "workdescriptor_fwd.hpp"
#include "copydata_decl.hpp"

namespace nanos {

    class ResilienceNode {
        bool _inUse;
        bool _computed;
        int _id;
        int _parent;
        int _desc;
        int _next;
        int _result;
        size_t _descSize;
        size_t _resultsSize;
        unsigned int _lastDescVisited;
        unsigned int _lastDescRestored;

        void addDesc( ResilienceNode * rn );
        void removeAllDescs();
        void addNext( ResilienceNode * rn );
        void removeNext( ResilienceNode * rn );

        public:
        ~ResilienceNode();
        void setParent( ResilienceNode * parent );
        ResilienceNode * getParent();
        size_t getNumDescendants();
        bool isComputed() const;
        void storeResult( CopyData * copies, size_t numCopies, int task_id );
        void loadResult( CopyData * copies, size_t numCopies, int task_id );
        void restartLastDescVisited();
        void restartLastDescRestored();
        //void restartAllLastDescVisited();
        //void restartAllLastDescRestored();
        bool isInUse() const;
        void setInUse( bool flag );
        int getId() const;
        void setId( int id );
        size_t getResultsSize() const;
        int getResultIndex() const;

        ResilienceNode* getNextDesc( bool inc = false );
        ResilienceNode* getNextDescToRestore( bool inc = false );

        void removeDesc( ResilienceNode * rn );
    };

}
#endif /* NANOS_RESILIENCE_DECL_H */
