// Copyright 2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// Implements the YangParseTreePaths::AddSubtreeVirtualPort() method and its
// supporting functions.

#include "gnmi/gnmi.pb.h"
#include "stratum/glue/logging.h"
#include "stratum/glue/status/status_macros.h"
#include "stratum/hal/lib/common/gnmi_events.h"
#include "stratum/hal/lib/common/gnmi_publisher.h"
#include "stratum/hal/lib/common/utils.h"
#include "stratum/hal/lib/yang/yang_parse_tree.h"
#include "stratum/hal/lib/yang/yang_parse_tree_helpers.h"
#include "stratum/hal/lib/yang/yang_parse_tree_paths.h"

namespace stratum {
namespace hal {

using namespace stratum::hal::yang::helpers;

namespace {

////////////////////////////////////////////////////////////////////////////////
// /virtual-ports/virtual-port/state/vsi
void SetUpVirtualPortFetchVSI(TreeNode* node, YangParseTree* tree) {

  auto poll_functor = [tree](const GnmiEvent& event, const ::gnmi::Path& path,
                             GnmiSubscribeStream* stream) {
    // Create a data retrieval request.
    DataRequest req;
    auto* request = req.add_requests()->mutable_vport_vsi();
    // In-place definition of method retrieving data from generic response
    // and saving into 'resp' local variable.
    uint32 resp{};
    DataResponseWriter writer([&resp](const DataResponse& in) {
      if (!in.has_vport_vsi()) return false;
      resp = in.vport_vsi().vsi();
      return true;
    });
    // Query the switch. The returned status is ignored as there is no way to
    // notify the controller that something went wrong. The error is logged when
    // it is created.
    tree->GetSwitchInterface()
        ->RetrieveValue(/*node_id*/ 0, req, &writer, /* details= */ nullptr)
        .IgnoreError();
    return SendResponse(GetResponse(path, resp), stream);
  };
  auto on_change_functor = UnsupportedFunc();
  node->SetOnPollHandler(poll_functor)
      ->SetOnTimerHandler(poll_functor)
      ->SetOnChangeHandler(on_change_functor);
}

////////////////////////////////////////////////////////////////////////////////
// /virtual-ports/virtual-port/state/oper-status
void SetUpVirtualPortFetchOperStatus(TreeNode* node, YangParseTree* tree) {

  // -- return unsupported func 
  auto poll_functor = UnsupportedFunc();
  node->SetOnTimerHandler(poll_functor)
      ->SetOnPollHandler(poll_functor)
      ->SetOnChangeHandler(poll_functor);

}

}  // namespace

//////////////////////////////
//  AddSubtreeVirtualPort   //
//////////////////////////////

void YangParseTreePaths::AddSubtreeVirtualPort(YangParseTree* tree) {
  TreeNode* node =
      tree->AddNode(GetPath("virtual-ports")("virtual-port")("state")("vsi")());
  SetUpVirtualPortFetchVSI(node, tree);
  node = tree->AddNode(GetPath("virtual-ports")("virtual-port")("state")("oper-status")());
  SetUpVirtualPortFetchOperStatus(node, tree);
}

}  // namespace hal
}  // namespace stratum
