// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/lib/tdi/tdi_fixed_function_manager.h"

#include <string>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest-message.h"
#include "gtest/gtest.h"
#include "stratum/glue/status/status_test_util.h"
#include "stratum/hal/lib/tdi/tdi_sde_mock.h"
#include "stratum/lib/test_utils/matchers.h"

#define IPSEC_CONFIG_SADB_TABLE_NAME \
  "ipsec-offload.ipsec-offload.sad.sad-entry.ipsec-sa-config"

namespace stratum {
namespace hal {
namespace tdi {

using ::testing::_;
using ::testing::ByMove;
using ::testing::Return;
using TableKeyInterface = TdiSdeInterface::TableKeyInterface;
using TableDataInterface = TdiSdeInterface::TableDataInterface;

MATCHER_P(DerivedFromStatus, status, "") {
  if (arg.error_code() != status.error_code()) {
    return false;
  }
  if (arg.error_message().find(status.error_message()) == std::string::npos) {
    *result_listener << "\nOriginal error string: \"" << status.error_message()
                     << "\" is missing from the actual status.";
    return false;
  }
  return true;
}

constexpr char kUnspOpTypeMsg[] =
    "Unsupported update type: 2147483647 for IPSEC SADB table.";
constexpr int kTdiRtTableId = 1;

class TdiFixedFunctionManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    sde_wrapper_mock_ = absl::make_unique<TdiSdeMock>();
    fixed_function_manager_ = TdiFixedFunctionManager::CreateInstance(
        OPERATION_MODE_STANDALONE, sde_wrapper_mock_.get(), kDevice1);
  }

  ::util::Status OpTypeNotSuppError() {
    return MAKE_ERROR(ERR_INTERNAL) << kUnspOpTypeMsg;
  }

  static constexpr int kDevice1 = 0;
  std::unique_ptr<TdiSdeMock> sde_wrapper_mock_;
  std::unique_ptr<TdiFixedFunctionManager> fixed_function_manager_;
};

/*
 * Validates WriteSadbEntry for op_type: ADD_ENTRY
 * IPsecSADBConfig object is populated with accepted param
 * values and we check if WriteSadbEntry is invoked properly
 */
TEST_F(TdiFixedFunctionManagerTest, WriteSadbEntryTestAddEntry) {
  std::string table_name = IPSEC_CONFIG_SADB_TABLE_NAME;
  const IPsecSadbConfigOp op_type = IPSEC_SADB_CONFIG_OP_ADD_ENTRY;

  // create IPsecSADBConfig
  IPsecSADBConfig sadb_config;
  sadb_config.set_offload_id(1);
  sadb_config.set_direction(true);
  sadb_config.set_req_id(1);
  sadb_config.set_spi(1);
  sadb_config.set_ext_seq_num(true);
  sadb_config.set_anti_replay_window_size(1);
  sadb_config.set_protocol_parameters(IPSEC_PROTOCOL_PARAMS_ESP);
  sadb_config.set_mode(IPSEC_MODE_TRANSPORT);

  // mocked session object
  auto session_mock = std::make_shared<SessionMock>();
  auto table_key_mock = absl::make_unique<TableKeyMock>();
  auto table_data_mock = absl::make_unique<TableDataMock>();

  // Specify the behavior of certain methods of the SDE Wrapper mock
  // when they are invoked by WriteSadbEntry().
  EXPECT_CALL(
      // mock object to monitor
      *sde_wrapper_mock_,
      // method signature to match
      GetTableId(table_name))
      // action to take on first and only call
      .WillOnce(Return(kTdiRtTableId));

  EXPECT_CALL(*sde_wrapper_mock_,
              InsertTableEntry(kDevice1, _, kTdiRtTableId, table_key_mock.get(),
                               table_data_mock.get()))
      .WillOnce(Return(::util::OkStatus()));

  EXPECT_CALL(*sde_wrapper_mock_, CreateTableKey(kTdiRtTableId))
      .WillOnce(
          Return(ByMove(::util::StatusOr<std::unique_ptr<TableKeyInterface>>(
              std::move(table_key_mock)))));

  EXPECT_CALL(*sde_wrapper_mock_, CreateTableData(kTdiRtTableId, _))
      .WillOnce(
          Return(ByMove(::util::StatusOr<std::unique_ptr<TableDataInterface>>(
              std::move(table_data_mock)))));

  // perform test and check results (act + assert)
  EXPECT_OK(fixed_function_manager_->WriteSadbEntry(session_mock, table_name,
                                                    op_type, sadb_config));
}

/*
 * Validates WriteSadbEntry for op_type: DEL_ENTRY
 * IPsecSADBConfig object is populated with accepted param
 * values and we check if WriteSadbEntry is invoked properly
 */
TEST_F(TdiFixedFunctionManagerTest, WriteSadbEntryTestDelEntry) {
  std::string table_name = IPSEC_CONFIG_SADB_TABLE_NAME;
  const IPsecSadbConfigOp op_type = IPSEC_SADB_CONFIG_OP_DEL_ENTRY;

  // create IPsecSADBConfig
  IPsecSADBConfig sadb_config;
  sadb_config.set_offload_id(1);
  sadb_config.set_direction(true);
  sadb_config.set_req_id(1);
  sadb_config.set_spi(1);
  sadb_config.set_ext_seq_num(true);
  sadb_config.set_anti_replay_window_size(1);
  sadb_config.set_protocol_parameters(IPSEC_PROTOCOL_PARAMS_ESP);
  sadb_config.set_mode(IPSEC_MODE_TRANSPORT);

  // mocked session object
  auto session_mock = std::make_shared<SessionMock>();
  auto table_key_mock = absl::make_unique<TableKeyMock>();
  auto table_data_mock = absl::make_unique<TableDataMock>();

  // Specify the behavior of certain methods of the SDE Wrapper mock
  // when they are invoked by WriteSadbEntry().
  EXPECT_CALL(
      // mock object to monitor
      *sde_wrapper_mock_,
      // method signature to match
      GetTableId(table_name))
      // action to take on first and only call
      .WillOnce(Return(kTdiRtTableId));

  EXPECT_CALL(*sde_wrapper_mock_, DeleteTableEntry(kDevice1, _, kTdiRtTableId,
                                                   table_key_mock.get()))
      .WillOnce(Return(::util::OkStatus()));

  EXPECT_CALL(*sde_wrapper_mock_, CreateTableKey(kTdiRtTableId))
      .WillOnce(
          Return(ByMove(::util::StatusOr<std::unique_ptr<TableKeyInterface>>(
              std::move(table_key_mock)))));

  EXPECT_CALL(*sde_wrapper_mock_, CreateTableData(kTdiRtTableId, _))
      .WillOnce(
          Return(ByMove(::util::StatusOr<std::unique_ptr<TableDataInterface>>(
              std::move(table_data_mock)))));

  // perform test and check results (act + assert)
  EXPECT_OK(fixed_function_manager_->WriteSadbEntry(session_mock, table_name,
                                                    op_type, sadb_config));
}

/*
 * Validates WriteSadbEntry for unsupported op_type
 * The function is supposed to return error with
 * Unsupported op_type.
 */
TEST_F(TdiFixedFunctionManagerTest, WriteSadbEntryTestUnsupportedType) {
  std::string table_name = IPSEC_CONFIG_SADB_TABLE_NAME;
  const IPsecSadbConfigOp op_type =
      IPsecSadbConfigOp_INT_MAX_SENTINEL_DO_NOT_USE_;

  // create IPsecSADBConfig
  IPsecSADBConfig sadb_config;
  sadb_config.set_offload_id(1);
  sadb_config.set_direction(true);
  sadb_config.set_req_id(1);
  sadb_config.set_spi(1);
  sadb_config.set_ext_seq_num(true);
  sadb_config.set_anti_replay_window_size(1);
  sadb_config.set_protocol_parameters(IPSEC_PROTOCOL_PARAMS_ESP);
  sadb_config.set_mode(IPSEC_MODE_TRANSPORT);

  // mocked session object
  auto session_mock = std::make_shared<SessionMock>();
  auto table_key_mock = absl::make_unique<TableKeyMock>();
  auto table_data_mock = absl::make_unique<TableDataMock>();

  // Specify the behavior of certain methods of the SDE Wrapper mock
  // when they are invoked by WriteSadbEntry().
  EXPECT_CALL(
      // mock object to monitor
      *sde_wrapper_mock_,
      // method signature to match
      GetTableId(table_name))
      // action to take on first and only call
      .WillOnce(Return(kTdiRtTableId));

  EXPECT_CALL(*sde_wrapper_mock_, CreateTableKey(kTdiRtTableId))
      .WillOnce(
          Return(ByMove(::util::StatusOr<std::unique_ptr<TableKeyInterface>>(
              std::move(table_key_mock)))));

  EXPECT_CALL(*sde_wrapper_mock_, CreateTableData(kTdiRtTableId, _))
      .WillOnce(
          Return(ByMove(::util::StatusOr<std::unique_ptr<TableDataInterface>>(
              std::move(table_data_mock)))));

  // perform test and check results (act + assert)
  EXPECT_THAT(fixed_function_manager_->WriteSadbEntry(session_mock, table_name,
                                                      op_type, sadb_config),
              DerivedFromStatus(OpTypeNotSuppError()));
}

/*
 * Validates FetchSpi method.
 */
TEST_F(TdiFixedFunctionManagerTest, FetchSpiTest) {
  std::string table_name = IPSEC_CONFIG_SADB_TABLE_NAME;
  uint32 fetched_spi;

  // mocked session object
  auto session_mock = std::make_shared<SessionMock>();
  auto table_data_mock = absl::make_unique<TableDataMock>();

  // Specify the behavior of certain methods of the SDE Wrapper mock
  // when they are invoked by WriteSadbEntry().
  EXPECT_CALL(
      // mock object to monitor
      *sde_wrapper_mock_,
      // method signature to match
      GetTableId(table_name))
      // action to take on first and only call
      .WillOnce(Return(kTdiRtTableId));

  EXPECT_CALL(*sde_wrapper_mock_, CreateTableData(kTdiRtTableId, _))
      .WillOnce(
          Return(ByMove(::util::StatusOr<std::unique_ptr<TableDataInterface>>(
              std::move(table_data_mock)))));

  EXPECT_OK(fixed_function_manager_->FetchSpi(session_mock, table_name,
                                              &fetched_spi));
}
}  // namespace tdi
}  // namespace hal
}  // namespace stratum
