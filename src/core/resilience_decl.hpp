#ifndef NANOS_RESILIENCE_DECL_H
#define NANOS_RESILIENCE_DECL_H

#include <list>
#include "workdescriptor_fwd.hpp"
#include "copydata_decl.hpp"

namespace nanos {

    class ResilienceNode {
        //ResilienceNode *_parent;
        //ResilienceNode *_descNode;
        //ResilienceNode *_next;
        //void *_result;
        /* - Now, pointers to other nodes are offsets from the start of sys._persistentResilienceTree.
         * - Now, pointers to results are offsets from the start of sys._persistentResilienceResults.
         */
        int _parent;
        int _desc;
        int _next;
        int _result;
        size_t _descSize;
        bool _computed;
        bool _inUse;
        unsigned int _lastDescVisited;

        void incLastDescVisited();

        void addDescNode( ResilienceNode * rn );
        void removeDescNode( ResilienceNode * rn );
        void removeAllDescsNode();
        void addNext( ResilienceNode * rn );
        void removeNext( ResilienceNode * rn );

        public:
        ResilienceNode( ResilienceNode * parent, CopyData * copies, size_t numCopies ); 
        ~ResilienceNode();
        void setParent( ResilienceNode * parent );
        ResilienceNode * getParent();
        bool isComputed() const;
        void storeResult( CopyData * copies, size_t numCopies );
        void loadResult( CopyData * copies, size_t numCopies );
        void restartLastDescVisited();
        void restartAllLastDescVisited();
        bool isInUse() const;
        void setInUse( bool flag );

        ResilienceNode* getCurrentDescNode();
    };

}
#endif /* NANOS_RESILIENCE_DECL_H */
