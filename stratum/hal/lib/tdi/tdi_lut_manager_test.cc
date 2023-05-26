// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stratum/hal/lib/tdi/tdi_lut_manager.h"
#include "stratum/hal/lib/tdi/tdi_constants.h"

#include <string>
#include <vector>

#include "absl/memory/memory.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "gtest/gtest-message.h"
#include "stratum/glue/status/status_test_util.h"
#include "stratum/hal/lib/tdi/tdi_sde_mock.h"
#include "stratum/lib/utils.h"

namespace stratum {
namespace hal {
namespace tdi {

using ::testing::_;
using ::testing::ByMove;
using ::testing::Return;

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

constexpr char kUnspExternMsg[] = "Unsupported extern type 0";
constexpr char kUnspReadOpMsg[] = "READ VLUT ENTRY IS YET TO BE SUPPORTED";
constexpr int kP4TableId = 1;
constexpr int kTdiRtTableId = 1;

class TdiLutManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        sde_wrapper_mock_ = absl::make_unique<TdiSdeMock>();
        tdi_lut_manager_ = TdiLutManager::CreateInstance(
            OPERATION_MODE_STANDALONE, sde_wrapper_mock_.get(), kDevice1);
    }

    ::util::Status InvalidParamError() {
        return MAKE_ERROR(ERR_INVALID_PARAM) << kUnspExternMsg;
    }

    ::util::Status InternalError() {
        return MAKE_ERROR(ERR_INTERNAL) << kUnspReadOpMsg;
    }

    static constexpr int kDevice1 = 0;

    std::unique_ptr<TdiSdeMock> sde_wrapper_mock_;
    std::unique_ptr<TdiLutManager> tdi_lut_manager_;
};

/*
* This test case intends to test ReadTableEntry function
* for extern_type = kMvlutExactMatch. Since it is not yet
* supported, we invoke the func with correct set of args
* and check if the funciton is invoked properly.
*/
TEST_F(TdiLutManagerTest, ReadTableEntryTestExactMatch) {
    // create extern entry
    ::p4::v1::ExternEntry extern_entry;
    extern_entry.set_extern_type_id(kMvlutExactMatch);
    extern_entry.set_extern_id(1);

    // create mvlut entry
    auto* mvlut_entry = new ::p4::v1::MvlutEntry();
    mvlut_entry->set_mvlut_id(1);
    mvlut_entry->add_match()->set_field_id(1);

    // define parameter and all to mvlut entry
    auto* param = new ::p4::v1::MvlutParam();
    param->add_params()->set_param_id(1);
    param->add_params()->set_value("000001");
    mvlut_entry->set_allocated_param(param);

    // pack mvlut entry to extern entry
    auto* any = new google::protobuf::Any();
    any->PackFrom(*mvlut_entry);
    extern_entry.set_allocated_entry(any);

    // set expectation
    EXPECT_CALL(*sde_wrapper_mock_, GetTdiRtId(kP4TableId))
        .WillOnce(Return(kTdiRtTableId));

    // perform test and check results (act + assert)
    auto session_mock = std::make_shared<SessionMock>();
    WriterInterface<::p4::v1::ReadResponse>* writer;

    EXPECT_THAT(tdi_lut_manager_->ReadTableEntry(session_mock, extern_entry,
        writer), DerivedFromStatus(InternalError()));
}

/*
* This test case intends to test ReadTableEntry function
* for extern_type = kMvlutTernaryMatch. Since it is not yet
* supported, we invoke the func with correct set of args
* and check if the funciton is invoked properly.
*/
TEST_F(TdiLutManagerTest, ReadTableEntryTestTernaryMatch) {
    // create extern entry
    ::p4::v1::ExternEntry extern_entry;
    extern_entry.set_extern_type_id(kMvlutTernaryMatch);
    extern_entry.set_extern_id(1);

    // create mvlut entry
    auto* mvlut_entry = new ::p4::v1::MvlutEntry();
    mvlut_entry->set_mvlut_id(1);
    mvlut_entry->add_match()->set_field_id(1);

    // define parameter and all to mvlut entry
    auto* param = new ::p4::v1::MvlutParam();
    param->add_params()->set_param_id(1);
    param->add_params()->set_value("000001");
    mvlut_entry->set_allocated_param(param);

    // pack mvlut entry to extern entry
    auto* any = new google::protobuf::Any();
    any->PackFrom(*mvlut_entry);
    extern_entry.set_allocated_entry(any);

    // set expectation
    EXPECT_CALL(*sde_wrapper_mock_, GetTdiRtId(kP4TableId))
        .WillOnce(Return(kTdiRtTableId));

    // perform test and check results (act + assert)
    auto session_mock = std::make_shared<SessionMock>();
    WriterInterface<::p4::v1::ReadResponse>* writer;

    EXPECT_THAT(tdi_lut_manager_->ReadTableEntry(session_mock, extern_entry,
        writer), DerivedFromStatus(InternalError()));
}

/*
* This test case intends to test ReadTableEntry function
* for undefined extern_type. We check if the function returns
* the error message of "Unssupported extern type."
*/
TEST_F(TdiLutManagerTest, ReadTableEntryTestUnsupportedType) {
    auto session_mock = std::make_shared<SessionMock>();
    WriterInterface<::p4::v1::ReadResponse>* writer;
    ::p4::v1::ExternEntry entry;

    // expect the func to return Invalid Param error in case extern id
    // is not set
    EXPECT_THAT(tdi_lut_manager_->ReadTableEntry(session_mock, entry,
        writer), DerivedFromStatus(InvalidParamError()));
}

/*
* This test case intends to test WriteTableEntry function
* for extern_type = kMvlutExactMatch. The operation is INSERT
* here. Extern entry is correctly populated with some values
* and we check if the return type is OK.
*/
TEST_F(TdiLutManagerTest, WriteTableEntryTestExactMatchInsert) {
    // create extern entry
    ::p4::v1::ExternEntry extern_entry;
    extern_entry.set_extern_type_id(kMvlutExactMatch);
    extern_entry.set_extern_id(1);

    // create mvlut entry
    auto* mvlut_entry = new ::p4::v1::MvlutEntry();
    mvlut_entry->set_mvlut_id(1);

    // define parameter and all to mvlut entry
    auto* param = new ::p4::v1::MvlutParam();
    param->add_params()->set_param_id(1);
    param->add_params()->set_value("000001");
    mvlut_entry->set_allocated_param(param);

    // define match_field and exact field
    auto match_field = mvlut_entry->add_match();
    auto* exact = new ::p4::v1::FieldMatch_Exact();
    exact->set_value("1");
    match_field->set_field_id(1);
    match_field->set_allocated_exact(exact);

    // pack mvlut entry to extern entry
    auto* any = new google::protobuf::Any();
    any->PackFrom(*mvlut_entry);
    extern_entry.set_allocated_entry(any);

    // mocked objects
    auto session_mock = std::make_shared<SessionMock>();
    auto table_key_mock = absl::make_unique<TableKeyMock>();
    auto table_data_mock = absl::make_unique<TableDataMock>();

    using TableKeyInterface = TdiSdeInterface::TableKeyInterface;
    using TableDataInterface = TdiSdeInterface::TableDataInterface;

    // set expectation
    EXPECT_CALL(
        // mock call to monitor
        *sde_wrapper_mock_,
        // method signature to match
        InsertTableEntry(kDevice1, _, kTdiRtTableId, table_key_mock.get(),
                         table_data_mock.get()))
        // action to take on first and only call
        .WillOnce(Return(::util::OkStatus()));

    // set expectation
    EXPECT_CALL(
        // mock call to monitor
        *sde_wrapper_mock_,
        // method signature to match
        CreateTableKey(kTdiRtTableId))
        // action to take on first and only call
        .WillOnce(Return(ByMove(
            ::util::StatusOr<std::unique_ptr<TableKeyInterface>>(
                std::move(table_key_mock)))));

    // set expectation
    EXPECT_CALL(
        // mock call to monitor
        *sde_wrapper_mock_,
        // method signature to match
        CreateTableData(kTdiRtTableId, _))
        // action to take on first and only call
        .WillOnce(Return(ByMove(
            ::util::StatusOr<std::unique_ptr<TableDataInterface>>(
                std::move(table_data_mock)))));

    // perform test and check results (act + assert)
    EXPECT_OK(tdi_lut_manager_->WriteTableEntry(session_mock,
        ::p4::v1::Update::INSERT, extern_entry));
}

/*
* This test case intends to test WriteTableEntry function
* for extern_type = kMvlutExactMatch. The operation is MODIFY
* here. Extern entry is correctly populated with some values
* and we check if the return type is OK.
*/
TEST_F(TdiLutManagerTest, WriteTableEntryTestExactMatchModify) {
    // create extern entry
    ::p4::v1::ExternEntry extern_entry;
    extern_entry.set_extern_type_id(kMvlutExactMatch);
    extern_entry.set_extern_id(1);

    // create mvlut entry
    auto* mvlut_entry = new ::p4::v1::MvlutEntry();
    mvlut_entry->set_mvlut_id(1);

    // define parameter and all to mvlut entry
    auto* param = new ::p4::v1::MvlutParam();
    param->add_params()->set_param_id(1);
    param->add_params()->set_value("000001");
    mvlut_entry->set_allocated_param(param);

    // define match_field and exact field
    auto match_field = mvlut_entry->add_match();
    auto* exact = new ::p4::v1::FieldMatch_Exact();
    exact->set_value("1");
    match_field->set_field_id(1);
    match_field->set_allocated_exact(exact);

    // pack mvlut entry to extern entry
    auto* any = new google::protobuf::Any();
    any->PackFrom(*mvlut_entry);
    extern_entry.set_allocated_entry(any);

    // mocked objects
    auto table_key_mock = absl::make_unique<TableKeyMock>();
    auto table_data_mock = absl::make_unique<TableDataMock>();
    auto session_mock = std::make_shared<SessionMock>();

    using TableKeyInterface = TdiSdeInterface::TableKeyInterface;
    using TableDataInterface = TdiSdeInterface::TableDataInterface;

    // set expectation
    EXPECT_CALL(
        // mock call to monitor
        *sde_wrapper_mock_,
        // method signature to match
        ModifyTableEntry(kDevice1, _, kTdiRtTableId, table_key_mock.get(),
                         table_data_mock.get()))
        // action to take on first and only call
        .WillOnce(Return(::util::OkStatus()));

    // set expectation
    EXPECT_CALL(
        // mock call to monitor
        *sde_wrapper_mock_,
        // method signature to match
        CreateTableKey(kTdiRtTableId))
        // action to take on first and only call
        .WillOnce(Return(ByMove(
            ::util::StatusOr<std::unique_ptr<TableKeyInterface>>(
                std::move(table_key_mock)))));

    // set expectation
    EXPECT_CALL(
        // mock call to monitor
        *sde_wrapper_mock_,
        // method signature to match
        CreateTableData(kTdiRtTableId, _))
        // action to take on first and only call
        .WillOnce(Return(ByMove(
            ::util::StatusOr<std::unique_ptr<TableDataInterface>>(
                std::move(table_data_mock)))));

    // perform test and check results (act + assert)
    EXPECT_OK(tdi_lut_manager_->WriteTableEntry(session_mock,
        ::p4::v1::Update::MODIFY, extern_entry));
}

/*
* This test case intends to test WriteTableEntry function
* for extern_type = kMvlutExactMatch. The operation is DELETE
* here. Extern entry is correctly populated with some values
* and we check if the return type is OK.
*/
TEST_F(TdiLutManagerTest, WriteTableEntryTestExactMatchDelete) {
    // create extern entry
    ::p4::v1::ExternEntry extern_entry;
    extern_entry.set_extern_type_id(kMvlutExactMatch);
    extern_entry.set_extern_id(1);

    // create mvlut entry
    auto* mvlut_entry = new ::p4::v1::MvlutEntry();
    mvlut_entry->set_mvlut_id(1);

    // define parameter and all to mvlut entry
    auto* param = new ::p4::v1::MvlutParam();
    param->add_params()->set_param_id(1);
    param->add_params()->set_value("000001");
    mvlut_entry->set_allocated_param(param);

    // define match_field and exact field
    auto match_field = mvlut_entry->add_match();
    auto* exact = new ::p4::v1::FieldMatch_Exact();
    exact->set_value("1");
    match_field->set_field_id(1);
    match_field->set_allocated_exact(exact);

    // pack mvlut entry to extern entry
    auto* any = new google::protobuf::Any();
    any->PackFrom(*mvlut_entry);
    extern_entry.set_allocated_entry(any);

    // mocked objects
    auto session_mock = std::make_shared<SessionMock>();
    auto table_key_mock = absl::make_unique<TableKeyMock>();
    auto table_data_mock = absl::make_unique<TableDataMock>();

    using TableKeyInterface = TdiSdeInterface::TableKeyInterface;
    using TableDataInterface = TdiSdeInterface::TableDataInterface;

    // set expectation
    EXPECT_CALL(
        // mock call to monitor
        *sde_wrapper_mock_,
        // method signature to match
        DeleteTableEntry(kDevice1, _, kTdiRtTableId,
                         table_key_mock.get()))
        // action to take on first and only call
        .WillOnce(Return(::util::OkStatus()));

    EXPECT_CALL(
        // mock call to monitor
        *sde_wrapper_mock_,
        // method signature to match
        CreateTableKey(kTdiRtTableId))
        // action to take on first and only call
        .WillOnce(Return(ByMove(
            ::util::StatusOr<std::unique_ptr<TableKeyInterface>>(
                std::move(table_key_mock)))));

    EXPECT_CALL(
        // mock call to monitor
        *sde_wrapper_mock_,
        // method signature to match
        CreateTableData(kTdiRtTableId, _))
        // action to take on first and only call
        .WillOnce(Return(ByMove(
            ::util::StatusOr<std::unique_ptr<TableDataInterface>>(
                std::move(table_data_mock)))));

    // perform test and check results (act + assert)
    EXPECT_OK(tdi_lut_manager_->WriteTableEntry(session_mock,
        ::p4::v1::Update::DELETE, extern_entry));
}

/*
* This test case intends to test WriteTableEntry function
* for extern_type = kMvlutTernaryMatch. The operation is INSERT
* here. Extern entry is correctly populated with some values
* and we check if the return type is OK.
*/
TEST_F(TdiLutManagerTest, WriteTableEntryTestTernaryMatchInsert) {
    // create extern entry
    ::p4::v1::ExternEntry extern_entry;
    extern_entry.set_extern_type_id(kMvlutTernaryMatch);
    extern_entry.set_extern_id(1);

    // create mvlut entry
    auto* mvlut_entry = new ::p4::v1::MvlutEntry();
    mvlut_entry->set_mvlut_id(1);

    // define parameter and all to mvlut entry
    auto* param = new ::p4::v1::MvlutParam();
    param->add_params()->set_param_id(1);
    param->add_params()->set_value("000001");
    mvlut_entry->set_allocated_param(param);

    // define match_field and ternary field
    auto match_field = mvlut_entry->add_match();
    auto* ternary = new ::p4::v1::FieldMatch_Ternary();
    ternary->set_value("1");
    ternary->set_mask("\xfff");
    match_field->set_field_id(1);
    match_field->set_allocated_ternary(ternary);

    // pack mvlut entry to extern entry
    auto* any = new google::protobuf::Any();
    any->PackFrom(*mvlut_entry);
    extern_entry.set_allocated_entry(any);

    // mocked objects
    auto table_key_mock = absl::make_unique<TableKeyMock>();
    auto table_data_mock = absl::make_unique<TableDataMock>();
    auto session_mock = std::make_shared<SessionMock>();

    using TableKeyInterface = TdiSdeInterface::TableKeyInterface;
    using TableDataInterface = TdiSdeInterface::TableDataInterface;

    // set expectation
    EXPECT_CALL(
        // mock call to monitor
        *sde_wrapper_mock_,
        // method signature to match
        InsertTableEntry(kDevice1, _, kTdiRtTableId, table_key_mock.get(),
                         table_data_mock.get()))
        // action to take on first and only call
        .WillOnce(Return(::util::OkStatus()));

    // set expectation
    EXPECT_CALL(
        // mock call to monitor
        *sde_wrapper_mock_,
        // method signature to match
        CreateTableKey(kTdiRtTableId))
        // action to take on first and only call
        .WillOnce(Return(ByMove(
            ::util::StatusOr<std::unique_ptr<TableKeyInterface>>(
                std::move(table_key_mock)))));

    // set expectation
    EXPECT_CALL(
        // mock call to monitor
        *sde_wrapper_mock_,
        // method signature to match
        CreateTableData(kTdiRtTableId, _))
        // action to take on first and only call
        .WillOnce(Return(ByMove(
            ::util::StatusOr<std::unique_ptr<TableDataInterface>>(
                std::move(table_data_mock)))));

    // perform test and check results (act + assert)
    EXPECT_OK(tdi_lut_manager_->WriteTableEntry(session_mock,
        ::p4::v1::Update::INSERT, extern_entry));
}

/*
* This test case intends to test WriteTableEntry function
* for extern_type = kMvlutTernaryMatch. The operation is MODIFY
* here. Extern entry is correctly populated with some values
* and we check if the return type is OK.
*/
TEST_F(TdiLutManagerTest, WriteTableEntryTestTernaryMatchModify) {
    // create extern entry
    ::p4::v1::ExternEntry extern_entry;
    extern_entry.set_extern_type_id(kMvlutTernaryMatch);
    extern_entry.set_extern_id(1);

    // create mvlut entry
    auto* mvlut_entry = new ::p4::v1::MvlutEntry();
    mvlut_entry->set_mvlut_id(1);

    // define parameter and all to mvlut entry
    auto* param = new ::p4::v1::MvlutParam();
    param->add_params()->set_param_id(1);
    param->add_params()->set_value("000001");
    mvlut_entry->set_allocated_param(param);

    // define match_field and ternary field
    auto match_field = mvlut_entry->add_match();
    auto* ternary = new ::p4::v1::FieldMatch_Ternary();
    ternary->set_value("1");
    ternary->set_mask("\xfff");
    match_field->set_field_id(1);
    match_field->set_allocated_ternary(ternary);

    // pack mvlut entry to extern entry
    auto* any = new google::protobuf::Any();
    any->PackFrom(*mvlut_entry);
    extern_entry.set_allocated_entry(any);

    // mocked objects
    auto session_mock = std::make_shared<SessionMock>();
    auto table_key_mock = absl::make_unique<TableKeyMock>();
    auto table_data_mock = absl::make_unique<TableDataMock>();

    using TableKeyInterface = TdiSdeInterface::TableKeyInterface;
    using TableDataInterface = TdiSdeInterface::TableDataInterface;

    // set expectation
    EXPECT_CALL(
        // mock call to monitor
        *sde_wrapper_mock_,
        // method signature to match
        ModifyTableEntry(kDevice1, _, kTdiRtTableId, table_key_mock.get(),
                         table_data_mock.get()))
        // action to take on first and only call
        .WillOnce(Return(::util::OkStatus()));

    // set expectation
    EXPECT_CALL(
        // mock call to monitor
        *sde_wrapper_mock_,
        // method signature to match
        CreateTableKey(kTdiRtTableId))
        // action to take on first and only call
        .WillOnce(Return(ByMove(
            ::util::StatusOr<std::unique_ptr<TableKeyInterface>>(
                std::move(table_key_mock)))));

    // set expectation
    EXPECT_CALL(
        // mock call to monitor
        *sde_wrapper_mock_,
        // method signature to match
        CreateTableData(kTdiRtTableId, _))
        .WillOnce(Return(ByMove(
        // action to take on first and only call
            ::util::StatusOr<std::unique_ptr<TableDataInterface>>(
                std::move(table_data_mock)))));

    // perform test and check results (act + assert)
    EXPECT_OK(tdi_lut_manager_->WriteTableEntry(session_mock,
        ::p4::v1::Update::MODIFY, extern_entry));
}

/*
* This test case intends to test WriteTableEntry function
* for extern_type = kMvlutTernaryMatch. The operation is DELETE
* here. Extern entry is correctly populated with some values
* and we check if the return type is OK.
*/
TEST_F(TdiLutManagerTest, WriteTableEntryTestTernaryMatchDelete) {
    // create extern entry
    ::p4::v1::ExternEntry extern_entry;
    extern_entry.set_extern_type_id(kMvlutTernaryMatch);
    extern_entry.set_extern_id(1);

    // create mvlut entry
    auto* mvlut_entry = new ::p4::v1::MvlutEntry();
    mvlut_entry->set_mvlut_id(1);

    // define parameter and all to mvlut entry
    auto* param = new ::p4::v1::MvlutParam();
    param->add_params()->set_param_id(1);
    param->add_params()->set_value("000001");
    mvlut_entry->set_allocated_param(param);

    // define match_field and ternary field
    auto match_field = mvlut_entry->add_match();
    auto* ternary = new ::p4::v1::FieldMatch_Ternary();
    ternary->set_value("1");
    ternary->set_mask("\xfff");
    match_field->set_field_id(1);
    match_field->set_allocated_ternary(ternary);

    // pack mvlut entry to extern entry
    auto* any = new google::protobuf::Any();
    any->PackFrom(*mvlut_entry);
    extern_entry.set_allocated_entry(any);

    // mocked objects
    auto session_mock = std::make_shared<SessionMock>();
    auto table_key_mock = absl::make_unique<TableKeyMock>();
    auto table_data_mock = absl::make_unique<TableDataMock>();

    using TableKeyInterface = TdiSdeInterface::TableKeyInterface;
    using TableDataInterface = TdiSdeInterface::TableDataInterface;

    // set expectation
    EXPECT_CALL(
        // mock call to monitor
        *sde_wrapper_mock_,
        // method signature to match
        DeleteTableEntry(kDevice1, _, kTdiRtTableId,
                         table_key_mock.get()))
        // action to take on first and only call
        .WillOnce(Return(::util::OkStatus()));

    // set expectation
    EXPECT_CALL(
        // mock call to monitor
        *sde_wrapper_mock_,
        // method signature to match
        CreateTableKey(kTdiRtTableId))
        // action to take on first and only call
        .WillOnce(Return(ByMove(
            ::util::StatusOr<std::unique_ptr<TableKeyInterface>>(
                std::move(table_key_mock)))));

    EXPECT_CALL(
        // mock call to monitor
        *sde_wrapper_mock_,
        // method signature to match
        CreateTableData(kTdiRtTableId, _))
        // action to take on first and only call
        .WillOnce(Return(ByMove(
            ::util::StatusOr<std::unique_ptr<TableDataInterface>>(
                std::move(table_data_mock)))));

    // perform test and check results (act + assert)
    EXPECT_OK(tdi_lut_manager_->WriteTableEntry(session_mock,
        ::p4::v1::Update::DELETE, extern_entry));
}

/*
* This test case intends to test WriteTableEntry function
* for undefined extern_type. We check if the function returns
* the error message of "Unssupported extern type."
*/
TEST_F(TdiLutManagerTest, WriteTableEntryTestUnsupportedType) {
    auto session_mock = std::make_shared<SessionMock>();
    ::p4::v1::ExternEntry extern_entry;
    ::p4::v1::Update::Type update_type = ::p4::v1::Update::INSERT;

    // expect the func to return Invalid Param Error in case extern id
    // is not set
    EXPECT_THAT(tdi_lut_manager_->WriteTableEntry(session_mock, update_type, extern_entry),
        DerivedFromStatus(InvalidParamError()));
}

}  // namespace tdi
}  // namespace hal
}  // namespace stratum
