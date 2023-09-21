// SPDX-License-Identifier: GPL-3.0-only

#ifndef RACCOON__POSTPROCESS__POSTPROCESS_HPP
#define RACCOON__POSTPROCESS__POSTPROCESS_HPP

namespace Raccoon::PostProcess {
    void set_up_pixelate_shader();

    inline void set_up_postprocess_effects() {
        try {
            set_up_pixelate_shader();
        }
        catch(...) {
            throw;
        }
    }
}

#endif
