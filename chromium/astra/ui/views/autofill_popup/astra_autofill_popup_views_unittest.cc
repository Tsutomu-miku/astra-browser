// Copyright 2025 The Astra Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "astra/ui/views/autofill_popup/astra_autofill_popup_view.h"

#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace astra {

class AstraAutofillPopupViewTest : public testing::Test {
 protected:
  void SetUp() override {}

  base::test::TaskEnvironment task_environment_;
};

// Test suggestion types enum.
TEST_F(AstraAutofillPopupViewTest, SuggestionTypes) {
  std::vector<AstraAutofillSuggestionType> types = {
      AstraAutofillSuggestionType::kAutofillProfile,
      AstraAutofillSuggestionType::kCreditCard,
      AstraAutofillSuggestionType::kPassword,
      AstraAutofillSuggestionType::kAddress,
      AstraAutofillSuggestionType::kEmail,
      AstraAutofillSuggestionType::kName,
      AstraAutofillSuggestionType::kPhone,
      AstraAutofillSuggestionType::kOrganization,
      AstraAutofillSuggestionType::kAutocomplete,
      AstraAutofillSuggestionType::kIban,
      AstraAutofillSuggestionType::kPlusAddress,
  };

  EXPECT_EQ(11u, types.size());
}

// Test suggestion struct.
TEST_F(AstraAutofillPopupViewTest, SuggestionStruct) {
  AstraAutofillSuggestion s;
  s.type = AstraAutofillSuggestionType::kEmail;
  s.main_text = u"test@example.com";
  s.secondary_text = u"Work email";
  s.suggestion_id = 42;
  s.deletable = true;

  EXPECT_EQ(AstraAutofillSuggestionType::kEmail, s.type);
  EXPECT_EQ(u"test@example.com", s.main_text);
  EXPECT_EQ(u"Work email", s.secondary_text);
  EXPECT_EQ(42, s.suggestion_id);
  EXPECT_TRUE(s.deletable);
  EXPECT_FALSE(s.is_separator);
  EXPECT_FALSE(s.is_instruction);
}

// Test separator suggestion.
TEST_F(AstraAutofillPopupViewTest, SeparatorSuggestion) {
  AstraAutofillSuggestion s;
  s.is_separator = true;
  EXPECT_TRUE(s.is_separator);
}

// Test instruction suggestion.
TEST_F(AstraAutofillPopupViewTest, InstructionSuggestion) {
  AstraAutofillSuggestion s;
  s.is_instruction = true;
  s.main_text = u"Manage addresses";
  EXPECT_TRUE(s.is_instruction);
  EXPECT_EQ(u"Manage addresses", s.main_text);
}

// Test credit card suggestion.
TEST_F(AstraAutofillPopupViewTest, CreditCardSuggestion) {
  AstraAutofillSuggestion s;
  s.type = AstraAutofillSuggestionType::kCreditCard;
  s.main_text = u"Visa •••• 4242";
  s.secondary_text = u"Exp: 12/28";
  s.card_network_label = u"Visa";
  s.card_color = SkColorSetRGB(0x1A, 0x73, 0xE8);

  EXPECT_EQ(u"Visa", s.card_network_label);
  EXPECT_EQ(SkColorSetRGB(0x1A, 0x73, 0xE8), s.card_color);
}

// Test password suggestion.
TEST_F(AstraAutofillPopupViewTest, PasswordSuggestion) {
  AstraAutofillSuggestion s;
  s.type = AstraAutofillSuggestionType::kPassword;
  s.main_text = u"user@example.com";
  s.username_label = u"Username";
  s.password_value = "••••••••";

  EXPECT_EQ(u"user@example.com", s.main_text);
  EXPECT_EQ(u"Username", s.username_label);
}

// Test popup basic creation.
TEST_F(AstraAutofillPopupViewTest, PopupCreation) {
  auto anchor = std::make_unique<views::View>();
  AstraAutofillPopupView popup(anchor.get());

  EXPECT_EQ(0u, popup.GetSuggestionCount());
  EXPECT_EQ(-1, popup.GetSelectedIndex());
  EXPECT_EQ(nullptr, popup.delegate());
}

// Test setting suggestions.
TEST_F(AstraAutofillPopupViewTest, SetSuggestions) {
  auto anchor = std::make_unique<views::View>();
  AstraAutofillPopupView popup(anchor.get());

  std::vector<AstraAutofillSuggestion> suggestions;

  AstraAutofillSuggestion s1;
  s1.type = AstraAutofillSuggestionType::kEmail;
  s1.main_text = u"user@example.com";
  s1.suggestion_id = 1;
  suggestions.push_back(s1);

  AstraAutofillSuggestion s2;
  s2.type = AstraAutofillSuggestionType::kEmail;
  s2.main_text = u"work@company.com";
  s2.suggestion_id = 2;
  suggestions.push_back(s2);

  popup.SetSuggestions(suggestions);
  EXPECT_EQ(2u, popup.GetSuggestionCount());
  EXPECT_EQ(0, popup.GetSelectedIndex());
}

// Test selection navigation.
TEST_F(AstraAutofillPopupViewTest, SelectionNavigation) {
  auto anchor = std::make_unique<views::View>();
  AstraAutofillPopupView popup(anchor.get());

  std::vector<AstraAutofillSuggestion> suggestions;
  for (int i = 0; i < 5; i++) {
    AstraAutofillSuggestion s;
    s.main_text = base::UTF8ToUTF16("Item " + std::to_string(i));
    s.suggestion_id = i;
    suggestions.push_back(s);
  }
  popup.SetSuggestions(suggestions);

  EXPECT_EQ(0, popup.GetSelectedIndex());

  popup.SelectNext();
  EXPECT_EQ(1, popup.GetSelectedIndex());

  popup.SelectPrevious();
  EXPECT_EQ(0, popup.GetSelectedIndex());

  // Wrap around.
  popup.SelectPrevious();
  EXPECT_EQ(4, popup.GetSelectedIndex());

  popup.SelectNext();
  EXPECT_EQ(0, popup.GetSelectedIndex());
}

// Test footer.
TEST_F(AstraAutofillPopupViewTest, Footer) {
  auto anchor = std::make_unique<views::View>();
  AstraAutofillPopupView popup(anchor.get());

  popup.SetFooterText(u"Manage addresses");
  popup.SetFooterVisible(true);
}

// Test scroll view exists.
TEST_F(AstraAutofillPopupViewTest, ScrollViewExists) {
  auto anchor = std::make_unique<views::View>();
  AstraAutofillPopupView popup(anchor.get());
  EXPECT_NE(nullptr, popup.scroll_view_for_test());
}

// Test delegate.
TEST_F(AstraAutofillPopupViewTest, Delegate) {
  class TestDelegate : public AstraAutofillPopupDelegate {
   public:
    int accepted_id = -1;
    bool closed = false;

    void OnAutofillSuggestionAccepted(int id) override {
      accepted_id = id;
    }
    void OnAutofillPopupClosed() override { closed = true; }
  };

  auto anchor = std::make_unique<views::View>();
  AstraAutofillPopupView popup(anchor.get());

  TestDelegate delegate;
  popup.SetDelegate(&delegate);
  EXPECT_EQ(&delegate, popup.delegate());
}

// Test suggestion view count matches.
TEST_F(AstraAutofillPopupViewTest, SuggestionViewCount) {
  auto anchor = std::make_unique<views::View>();
  AstraAutofillPopupView popup(anchor.get());

  std::vector<AstraAutofillSuggestion> suggestions;
  for (int i = 0; i < 3; i++) {
    AstraAutofillSuggestion s;
    s.main_text = u"Test";
    suggestions.push_back(s);
  }
  popup.SetSuggestions(suggestions);
  EXPECT_EQ(3u, popup.GetSuggestionViewCount());
}

}  // namespace astra
