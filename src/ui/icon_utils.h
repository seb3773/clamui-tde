#ifndef CLAM_ICON_UTILS_H
#define CLAM_ICON_UTILS_H

#include <ntqpixmap.h>
#include <ntqimage.h>
#include "embedded_icons.h"

#include <tdeconfig.h>
#include <tdeglobal.h>
#include <map>

class IconUtils {
private:
    struct CacheKey {
        const unsigned char* data;
        unsigned int len;
        int w;
        int h;
        bool invert;
        
        bool operator<(const CacheKey& o) const {
            if (data != o.data) return data < o.data;
            if (len != o.len) return len < o.len;
            if (w != o.w) return w < o.w;
            if (h != o.h) return h < o.h;
            return invert < o.invert;
        }
    };
    
    static std::map<CacheKey, TQPixmap>& cache() {
        static std::map<CacheKey, TQPixmap> instance;
        return instance;
    }

public:
    static bool isDarkMode() {
        TDEConfig *config = TDEGlobal::config();
        config->setGroup("General");
        return config->readEntry("dark_mode", "false") == "true";
    }

private:

    static bool shouldInvert(unsigned int len) {
        if (!isDarkMode()) return false;
        
        // Excluded icons by length to avoid translation-unit pointer mismatch
        if (len == about_clamuitde_png_len ||
            len == main_icon_png_len ||
            len == scan_icon1_png_len ||
            len == scan_icon2_png_len ||
            len == scan_icon3_png_len ||
            len == scan_icon4_png_len ||
            len == scan_icon5_png_len ||
            len == secu_fail_png_len ||
            len == secu_ok_png_len ||
            len == tip_png_len) {
            return false;
        }
        return true;
    }

public:
    static void clearCache() {
        cache().clear();
        TDEGlobal::config()->reparseConfiguration();
    }

    static TQPixmap load(const unsigned char* data, unsigned int len, int w = -1, int h = -1) {
        bool invert = shouldInvert(len);
        CacheKey k = { data, len, w, h, invert };
        
        std::map<CacheKey, TQPixmap>& c = cache();
        if (c.find(k) != c.end()) {
            return c[k];
        }

        TQImage img;
        img.loadFromData(data, len, "PNG");
        if (w > 0 && h > 0 && (img.width() != w || img.height() != h)) {
            img = img.smoothScale(w, h);
        }
        
        if (invert) {
            if (img.depth() != 32) img = img.convertDepth(32);
            if (!img.hasAlphaBuffer()) img.setAlphaBuffer(true);
            
            for (int y = 0; y < img.height(); ++y) {
                TQRgb *line = (TQRgb*)img.scanLine(y);
                for (int x = 0; x < img.width(); ++x) {
                    int alpha = tqAlpha(line[x]);
                    int r = 255 - tqRed(line[x]);
                    int g = 255 - tqGreen(line[x]);
                    int b = 255 - tqBlue(line[x]);
                    line[x] = tqRgba(r, g, b, alpha);
                }
            }
        }
        
        TQPixmap pm(img);
        c[k] = pm;
        return pm;
    }
    
    // To colorize a flat mask icon (like a black and white icon with transparency)
    static TQPixmap colorize(const unsigned char* data, unsigned int len, const TQColor &color, int w = -1, int h = -1) {
        // We do not cache colorized icons yet, or we could add a different cache.
        TQImage img;
        img.loadFromData(data, len, "PNG");
        if (w > 0 && h > 0 && (img.width() != w || img.height() != h)) {
            img = img.smoothScale(w, h);
        }
        if (img.depth() != 32) {
            img = img.convertDepth(32);
        }
        if (!img.hasAlphaBuffer()) {
            img.setAlphaBuffer(true);
        }
        
        for (int y = 0; y < img.height(); ++y) {
            TQRgb *line = (TQRgb*)img.scanLine(y);
            for (int x = 0; x < img.width(); ++x) {
                int alpha = tqAlpha(line[x]);
                line[x] = tqRgba(color.red(), color.green(), color.blue(), alpha);
            }
        }
        return TQPixmap(img);
    }
};

#endif /* CLAM_ICON_UTILS_H */
