#pragma once
#include "property.hpp"
#include "executors/Blocking.hpp"
#include "executors/Directionality.hpp"
#include "executors/Relationship.hpp"
#include "executors/Mapping.hpp"
#include "executors/Outstanding_work.hpp"
#include "executors/Context.hpp"
#include "executors/Static_thread_pool.hpp"
#include "executors/Polymorphic_executor.hpp"

// Notes on execution:
//
// An executor type shall satisfy the requirements of CopyConstructible, 
// Destructible and EqualityComparable
//
// An executor type's destructor shall not block pending completion of
// the submitted function objects.
