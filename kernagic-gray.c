#include "kernagic.h"
#include "kerner.h"

void gray_each (Glyph *lg, GtkProgressBar *progress)
{
  GList *right;
  for (right = kernagic_glyphs (); right; right = right->next)
    {
      Glyph *rg = right->data;
      if (progress || kernagic_deal_with_glyphs (lg->unicode, rg->unicode))
        {
          /*XXX: kerner kern is wrong, this is method specific */
          float kerned_advance = kerner_kern (&kerner_settings, lg, rg);

          lg->right_bearing = 0;
          lg->left_bearing = 0;
          kernagic_set_kerning (lg, rg, kerned_advance - kernagic_get_advance (lg));
        }
    }
}

void recompute_right_bearings (void);

static void done (void)
{
  recompute_right_bearings ();
}

static KernagicMethod method = {"gray", 
  NULL,  gray_each, (void*)done};

KernagicMethod *kernagic_gray = &method;

