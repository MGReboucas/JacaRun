/****************************************************************************
Copyright (c) 2015-2016 Chukong Technologies Inc.
Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.
Copyright (c) 2019-present Axmol Engine contributors (see AUTHORS.md).

https://axmol.dev/

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/
package dev.axmol.app;

import android.os.Bundle;
import dev.axmol.lib.AxmolActivity;
import dev.axmol.lib.SharedLoader;
import android.os.Build;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;

public class AppActivity extends AxmolActivity {
    // Invoked on a native worker thread. Never override the platform trust manager or hostname verifier.
    public static String shopRequest(String address, String token, String body, String key) {
        javax.net.ssl.HttpsURLConnection connection = null;
        try {
            java.net.URL url = new java.net.URL(address);
            if (!"https".equals(url.getProtocol()) || url.getUserInfo() != null) return "";
            connection = (javax.net.ssl.HttpsURLConnection) url.openConnection();
            connection.setInstanceFollowRedirects(false);
            connection.setConnectTimeout(10000);
            connection.setReadTimeout(15000);
            connection.setRequestProperty("Content-Type", "application/json");
            connection.setRequestProperty("Authorization", "Bearer " + token);
            connection.setRequestProperty("Idempotency-Key", key);
            if (!body.isEmpty()) {
                connection.setRequestMethod("POST");
                connection.setDoOutput(true);
                try (java.io.OutputStream out = connection.getOutputStream()) {
                    out.write(body.getBytes(java.nio.charset.StandardCharsets.UTF_8));
                }
            }
            int status = connection.getResponseCode();
            if (status < 200 || status >= 300) return "";
            try (java.io.InputStream in = connection.getInputStream(); java.io.ByteArrayOutputStream out = new java.io.ByteArrayOutputStream()) {
                byte[] buffer = new byte[4096]; int n;
                while ((n = in.read(buffer)) != -1) {
                    out.write(buffer, 0, n);
                    if (out.size() > 65536) return "";
                }
                return out.toString("UTF-8");
            }
        } catch (Exception ignored) { return ""; }
        finally { if (connection != null) connection.disconnect(); }
    }
    static {
        // DNT remove, some android simulator require explicit load shared libraries, otherwise will crash
        SharedLoader.load();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.setEnableVirtualButton(false);
        super.onCreate(savedInstanceState);
        // Workaround in https://stackoverflow.com/questions/16283079/re-launch-of-activity-on-home-button-but-only-the-first-time/16447508
        if (!isTaskRoot()) {
            // Android launched another instance of the root activity into an existing task
            //  so just quietly finish and go away, dropping the user back into the activity
            //  at the top of the stack (ie: the last state of this task)
            // Don't need to finish it again since it's finished in super.onCreate .
            return;
        }
        // Make sure we're running on Pie or higher to change cutout mode
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            // Enable rendering into the cutout area
            WindowManager.LayoutParams lp = getWindow().getAttributes();
            lp.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
            getWindow().setAttributes(lp);
        }
        // DO OTHER INITIALIZATION BELOW

    }

}
