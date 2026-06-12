#include "astra/browser/astra_tab_features.h"

#include "content/public/browser/web_contents.h"

namespace astra {

AstraTabFeatures::AstraTabFeatures(content::WebContents* web_contents)
    : content::WebContentsUserData<AstraTabFeatures>(*web_contents) {}

AstraTabFeatures::~AstraTabFeatures() = default;

WEB_CONTENTS_USER_DATA_KEY_IMPL(AstraTabFeatures);

}  // namespace astra
