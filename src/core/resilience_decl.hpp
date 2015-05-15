#ifndef NANOS_RESILIENCE_DECL_H
#define NANOS_RESILIENCE_DECL_H

#include "copydata_decl.hpp"
#include "resilience_fwd.hpp"
#include <queue>
#include <list>
#include "atomic_decl.hpp"

namespace nanos {

    class ResiliencePersistence {

        // RELATED TO RESILIENCE TREE
        ResilienceNode * _resilienceTree;
        Lock _resilienceTreeLock;
        std::queue<int> _freeResilienceNodes;
        std::list<int> _usedResilienceNodes;
        int _resilienceTreeFileDescriptor;
        char * _resilienceTreeFilepath;

        // RELATED TO RESILIENCE RESULTS
        void * _resilienceResults;
        Atomic<void *> _freeResilienceResults;
        int _resilienceResultsFileDescriptor;
        char * _resilienceResultsFilepath;

        // COMMON
        size_t _RESILIENCE_MAX_FILE_SIZE;

        public:
        ResiliencePersistence();
        ~ResiliencePersistence();
        // RELATED TO RESILIENCE TREE
        ResilienceNode * getFreeResilienceNode();
        ResilienceNode * getResilienceNode( int offset );
        void freeResilienceNode( int index );

        // RELATED TO RESILIENCE RESULTS
        void * getResilienceResultsFreeSpace( size_t size );
        void * getResilienceResults( int offset );

    };

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

        public:
        void setParent( ResilienceNode * parent );
        ResilienceNode * getParent();
        size_t getNumDescendants();
        bool isComputed() const;
        void storeResult( CopyData * copies, size_t numCopies, int task_id );
        void loadResult( CopyData * copies, size_t numCopies, int task_id );
        void restartLastDescVisited();
        void restartLastDescRestored();
        bool isInUse() const;
        void setInUse( bool flag );
        int getId() const;
        void setId( int id );
        size_t getResultsSize() const;
        int getResultIndex() const;

        ResilienceNode* getNextDesc( bool inc = false );
        ResilienceNode* getNextDescToRestore( bool inc = false );

    };

}
#endif /* NANOS_RESILIENCE_DECL_H */
