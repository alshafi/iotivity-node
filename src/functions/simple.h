#ifndef __IOTIVITY_NODE_FUNCTIONS_SIMPLE_H__
#define __IOTIVITY_NODE_FUNCTIONS_SIMPLE_H__

#include <v8.h>

NAN_METHOD( bind_OCInit );
NAN_METHOD( bind_OCStop );
NAN_METHOD( bind_OCProcess );
NAN_METHOD( bind_OCStartPresence );
NAN_METHOD( bind_OCStopPresence );

#endif /* __IOTIVITY_NODE_FUNCTIONS_SIMPLE_H__ */
