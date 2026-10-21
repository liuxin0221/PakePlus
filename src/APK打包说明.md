# 学习备忘录 · Android 本地打包说明

把桌面 `自用备忘录` 文件夹里的网页打成离线 APK（入口用最新的 `学习错题相册.html`，10/18 的 `index.html` 也一并打进包里留档）。全程本地构建，不使用任何在线打包服务。

- 应用名：学习备忘录
- 包名：`com.memo.album`
- 版本：1.0（versionCode 1）
- 支持：Android 7.0（API 24）及以上
- 权限：**没有申请 INTERNET 权限**，页面与资源全部来自 APK 内的 assets，纯离线
- 图标：按网页启动屏的夜空配色重绘（原 `app_icon.png` 实际是一张“图片生成中”的占位图）

---

## 1. 为什么用 Android WebView 壳，而不是 Pake

| | Pake（Tauri） | Android WebView 壳 |
|---|---|---|
| 产物 | Windows / macOS / Linux 桌面程序 | Android APK |
| Android 支持 | 官方没有 Android 打包目标 | 原生支持 |
| 额外工具链 | Rust + cargo + Tauri CLI | 只要 JDK + Android SDK |
| 本机情况 | 未安装 Rust，且 GitHub 下载不稳定 | 已装好 JDK 17 + SDK 34，构建 34 秒完成 |

Pake 本质是「Tauri 桌面壳 + 网页」，它的发布流程只出桌面安装包；要拿 Tauri 打 Android 得自己 `tauri android init`、装 NDK、改 Gradle，绕一圈还是回到 Android 工程。所以这里直接用 WebView 壳，体积也只有 695 KB。

---

## 2. 环境（本机已装好，路径如下）

| 组件 | 版本 | 路径 |
|---|---|---|
| JDK | Microsoft OpenJDK 17.0.20.1 | `C:\Users\97603\AndroidDev\jdk17` |
| Android SDK | cmdline-tools 12.0 / platform 34 / build-tools 34.0.0 / platform-tools 37.0.1 | `C:\Users\97603\AndroidDev\sdk` |
| Gradle | 8.7 | `C:\Users\97603\AndroidDev\gradle-8.7` |
| 项目 | — | `C:\Users\97603\AndroidDev\memo-app` |

换电脑时按下面顺序重装（全部免安装、不需要管理员权限）：

```powershell
# 0) 建目录
New-Item -ItemType Directory -Force C:\Users\97603\AndroidDev\downloads | Out-Null

# 1) JDK 17（微软 OpenJDK，zip 免安装）
curl.exe -L -o C:\Users\97603\AndroidDev\downloads\jdk17.zip https://aka.ms/download-jdk/microsoft-jdk-17-windows-x64.zip
Expand-Archive C:\Users\97603\AndroidDev\downloads\jdk17.zip C:\Users\97603\AndroidDev\jdk-tmp
Move-Item (Get-ChildItem C:\Users\97603\AndroidDev\jdk-tmp -Directory)[0].FullName C:\Users\97603\AndroidDev\jdk17
Remove-Item -Recurse -Force C:\Users\97603\AndroidDev\jdk-tmp

# 2) Android 命令行工具
curl.exe -L -o C:\Users\97603\AndroidDev\downloads\cmdline-tools.zip https://dl.google.com/android/repository/commandlinetools-win-11076708_latest.zip
Expand-Archive C:\Users\97603\AndroidDev\downloads\cmdline-tools.zip C:\Users\97603\AndroidDev\ctl
New-Item -ItemType Directory -Force C:\Users\97603\AndroidDev\sdk\cmdline-tools | Out-Null
Move-Item C:\Users\97603\AndroidDev\ctl\cmdline-tools C:\Users\97603\AndroidDev\sdk\cmdline-tools\latest

# 3) 接受协议 + 安装 SDK 包
$env:JAVA_HOME='C:\Users\97603\AndroidDev\jdk17'
$env:PATH="$env:JAVA_HOME\bin;$env:PATH"
1..40 | ForEach-Object {'y'} | & C:\Users\97603\AndroidDev\sdk\cmdline-tools\latest\bin\sdkmanager.bat --sdk_root=C:\Users\97603\AndroidDev\sdk --licenses
& C:\Users\97603\AndroidDev\sdk\cmdline-tools\latest\bin\sdkmanager.bat --sdk_root=C:\Users\97603\AndroidDev\sdk "platform-tools" "platforms;android-34" "build-tools;34.0.0"

# 4) Gradle 8.7（官方源在国内很慢，用腾讯镜像）
curl.exe -L -o C:\Users\97603\AndroidDev\downloads\gradle-8.7-bin.zip https://mirrors.cloud.tencent.com/gradle/gradle-8.7-bin.zip
Expand-Archive C:\Users\97603\AndroidDev\downloads\gradle-8.7-bin.zip C:\Users\97603\AndroidDev
```

> 不需要安装 Android Studio；`local.properties` 已经指向 SDK，Gradle 会自己找。

---

## 3. 项目结构

```
memo-app\
├─ settings.gradle            仓库地址（阿里云镜像优先，官方源兜底）
├─ build.gradle               AGP 8.5.2
├─ gradle.properties          AndroidX、JVM 参数
├─ local.properties           sdk.dir
├─ build-debug.ps1            一键构建脚本
├─ gradlew.bat / gradle\       Gradle Wrapper（distributionUrl 指向腾讯镜像）
└─ app\
   ├─ build.gradle            包名、版本、依赖
   └─ src\main\
      ├─ AndroidManifest.xml  应用名/图标/主题，未声明 INTERNET
      ├─ assets\index.html    你的网页（原样拷贝）
      ├─ java\com\memo\album\MainActivity.java   WebView 外壳
      └─ res\                 strings.xml / themes.xml / 图标
```

### 网页是怎么加载的

`MainActivity` 用 `WebViewAssetLoader` 把 `assets/` 映射成

```
https://appassets.androidplatform.net/assets/index.html
```

而不是 `file:///android_asset/...`。原因：`file://` 是不透明来源，`localStorage`、`IndexedDB` 在部分 WebView 版本上会被禁用或互相隔离；这个假 https 域名有稳定的安全来源，你现在的「图片存 IndexedDB、备忘录存 localStorage」才能正常持久化，也不需要联网（请求在本地被拦截，不会走网络）。

外壳额外补了 WebView 默认没有的两件事：

1. `<input type="file">` 选图 / 选 JSON —— `onShowFileChooser` + 系统文件选择器，支持多选；
2. 「导出备份」—— 网页用 `blob:` + `<a download>` 触发下载，WebView 不支持，注入脚本拦截后交给原生保存到 `下载/学习备忘录/`。

---

## 4. 改网页 / 加资源

```powershell
# 改完网页后，重新拷贝进工程（入口固定叫 assets\index.html）
Copy-Item "C:\Users\97603\Desktop\自用备忘录\学习错题相册.html" `
          "C:\Users\97603\AndroidDev\memo-app\app\src\main\assets\index.html" -Force
```

> 文件夹里有两份网页：`学习错题相册.html`（10/20，新版：备忘录附图、科目快捷标签、手机端分类栏修复）和 `index.html`（10/18，旧版）。当前 APK 用新版做入口，旧版以 `assets\index-20251018.html` 一起打进包。想切回旧版：
>
> ```powershell
> Copy-Item "...\assets\index-20251018.html" "...\assets\index.html" -Force
> ```
>
> 然后重新打包。

有多个文件（css/js/图片）就一起放进 `app\src\main\assets\`，网页里用**相对路径**引用，例如：

```html
<link rel="stylesheet" href="style.css">
<script src="app.js"></script>
<img src="img/logo.png">
```

入口文件名必须是 `index.html`（或改 `MainActivity.APP_URL`）。

---

## 5. 改应用名

编辑 `app\src\main\res\values\strings.xml`：

```xml
<string name="app_name">学习备忘录</string>
```

改完重新构建即可，桌面上显示的就是这个名字。

## 6. 改包名

包名出现在两个地方：`app\build.gradle` 的 `namespace` 和 `applicationId`，以及 Java 的 `package` 声明 + 目录名。用下面的脚本一次改完（把 `com.yourname.memo` 换成你要的）：

```powershell
$root = 'C:\Users\97603\AndroidDev\memo-app'
$old  = 'com.memo.album'
$new  = 'com.yourname.memo'

# build.gradle 里两处
$f = "$root\app\build.gradle"
[IO.File]::WriteAllText($f, [IO.File]::ReadAllText($f).Replace($old, $new), [Text.UTF8Encoding]::new($false))

# Java package 声明
$j = "$root\app\src\main\java\com\memo\album\MainActivity.java"
[IO.File]::WriteAllText($j, [IO.File]::ReadAllText($j).Replace("package $old;", "package $new;"), [Text.UTF8Encoding]::new($false))

# 目录跟着改
$dst = Join-Path "$root\app\src\main\java" ($new.Replace('.', '\'))
New-Item -ItemType Directory -Force (Split-Path $dst) | Out-Null
Move-Item "$root\app\src\main\java\com\memo\album" $dst
```

注意：

- 包名只能用小写字母、数字、下划线，至少两段，不能以数字开头（如 `com.liu.memo` 合法，`com.2liu` 非法）。
- 改了 `applicationId` 等于换了一个 App，手机上会并存两个图标，旧数据不会迁移。
- 只是自己用的话，`com.memo.album` 完全可以不改；上架应用商店才必须唯一。

## 7. 打 debug 包

```powershell
cd C:\Users\97603\AndroidDev\memo-app
.\build-debug.ps1
```

脚本等价于：

```powershell
$env:JAVA_HOME='C:\Users\97603\AndroidDev\jdk17'
$env:ANDROID_HOME='C:\Users\97603\AndroidDev\sdk'
$env:PATH="$env:JAVA_HOME\bin;$env:ANDROID_HOME\platform-tools;$env:PATH"
C:\Users\97603\AndroidDev\gradle-8.7\bin\gradle.bat assembleDebug
```

首次构建会下载 AGP 和 AndroidX 依赖（约 2 分钟），之后改网页再打包一般 5～10 秒。

## 8. APK 输出位置

```
C:\Users\97603\AndroidDev\memo-app\app\build\outputs\apk\debug\app-debug.apk
```

已复制一份到桌面备忘录文件夹：

```
C:\Users\97603\Desktop\自用备忘录\学习备忘录-v1.0-debug.apk
```

- 大小：695 KB
- 签名：Android Debug（`CN=Android Debug`），可以直接侧载安装
- SHA-256：`2C99516C9032CED163D841804B4EF2D566ADFF11C254E584DEA54D1137B78258`

## 9. 安装到手机

### 方式 A：USB 数据线（推荐，能看日志）

1. 手机「设置 → 关于手机 → 连续点 7 次版本号」打开开发者选项；
2. 「开发者选项 → USB 调试」打开；
3. 数据线连电脑，手机上点「允许 USB 调试」；
4. 执行：

```powershell
$adb = 'C:\Users\97603\AndroidDev\sdk\platform-tools\adb.exe'
& $adb devices                      # 应能看到你的设备序列号
& $adb install -r "C:\Users\97603\Desktop\自用备忘录\学习备忘录-v1.0-debug.apk"
```

`Success` 即安装完成，桌面会出现「学习备忘录」图标。

### 方式 B：不插线

把 `学习备忘录-v1.0-debug.apk` 用微信/QQ/数据线/网盘传到手机，用文件管理器点开安装；系统提示「禁止安装未知应用」时，允许对应文件管理器安装即可。

### 卸载 / 覆盖安装

```powershell
& $adb uninstall com.memo.album
```

> 注意：卸载或「清除数据」会连同 IndexedDB 里的图片、localStorage 里的备忘录一起删掉。换手机/重装前，先在 App 里点「导出备份」，把 JSON 存到「下载/学习备忘录/」，装好后用「导入恢复」导回去。

## 10. 换图标

桌面上的 `app_icon.png` 打开后是一张“The image is generating…”的占位图（1832×1832，主色 `#EEF0F3`），直接拿来当图标会显示成灰色占位块，所以我按你网页启动屏的配色重绘了一版（深蓝夜空 + 月牙 + 「记」字），母版存在：

```
C:\Users\97603\AndroidDev\memo-app\icon-master.png
```

想换成自己的图：

```powershell
# 1) 准备一张 1024×1024 的 PNG，覆盖 icon-master.png
# 2) 重新生成各尺寸（脚本里把 add_moon/add_glyph 两行删掉，直接 resize 你的图即可）
& "C:\Users\97603\.dsh\dsh-runtimes\dsh-primary-runtime\dependencies\python\python.exe" `
  "C:\Users\97603\AndroidDev\make_icon_v2.py"
```

或者最省事：直接把 `res\mipmap-*\ic_launcher.png` 和 `ic_launcher_round.png` 换成你的图（同名同尺寸），再重新构建。

自适应图标（Android 8+）用 `res\mipmap-anydpi-v26\ic_launcher.xml`，背景/前景在 `res\drawable-nodpi\`。

## 11. 可选：自己签一个 release 包

debug 签名证书在 `C:\Users\97603\.android\debug.keystore`，换电脑就变了，不适合长期分发。要固定签名：

```powershell
& 'C:\Users\97603\AndroidDev\jdk17\bin\keytool.exe' -genkeypair -v `
  -keystore C:\Users\97603\AndroidDev\memo-release.jks `
  -alias memo -keyalg RSA -keysize 2048 -validity 10000 `
  -storepass 你的密码 -keypass 你的密码 `
  -dname "CN=memo, OU=self, O=self, L=Beijing, ST=Beijing, C=CN"
```

然后在 `app\build.gradle` 的 `android { }` 里加：

```groovy
signingConfigs {
    release {
        storeFile file('C:/Users/97603/AndroidDev/memo-release.jks')
        storePassword '你的密码'
        keyAlias 'memo'
        keyPassword '你的密码'
    }
}
buildTypes {
    release {
        signingConfig signingConfigs.release
        minifyEnabled false
    }
}
```

再执行 `gradle assembleRelease`，产物在 `app\build\outputs\apk\release\app-release.apk`。
长期自用其实 debug 包就够了。

## 12. 手机扫码下载（不插线）

电脑上跑一个临时 HTTP 服务，手机同 Wi-Fi 扫码即可下载，不经过任何网盘/在线服务。

```powershell
# 一键：复制 APK 到共享目录 + 按当前 IP 生成二维码 + 需要时启动服务
& "C:\Users\97603\Desktop\自用备忘录\生成下载二维码.ps1"
```

脚本做的事：

1. 把 APK 复制成 `C:\Users\97603\AndroidDev\share\memo-v1.0.apk`（纯英文名，避免扫码后文件名乱码）；
2. 取默认路由网卡的 IPv4，启动 `python -m http.server 8000`；
3. 生成 `C:\Users\97603\Desktop\自用备忘录\扫码下载.png` 并打开。

手动版：

```powershell
$py='C:\Users\97603\.dsh\dsh-runtimes\dsh-primary-runtime\dependencies\python\python.exe'
Start-Process $py -ArgumentList '-m','http.server','8000','--bind','0.0.0.0','--directory','C:\Users\97603\AndroidDev\share'
& $py "C:\Users\97603\AndroidDev\memo-app\make_qr.py" "http://192.168.x.x:8000/memo-v1.0.apk" "C:\Users\97603\Desktop\自用备忘录\扫码下载.png"
```

注意：

- 电脑和手机必须在**同一个局域网**；换了 Wi-Fi/热点，IP 会变，重跑脚本即可。
- 校园网/公司网常开「AP 隔离」，手机可能连不上电脑。这时让手机开热点、电脑连手机热点，再重跑脚本。
- 手机打不开时看 Windows 是否弹了「允许访问」的防火墙提示，勾选「专用 + 公用」。
- 下载完记得关掉服务（关闭那个 python 进程），或直接关机。

## 13. 常见问题

| 现象 | 处理 |
|---|---|
| 打开是白屏 | 确认 `assets\index.html` 存在；`adb logcat | Select-String "chromium"` 看报错 |
| 网页里的图片/样式 404 | 资源要放进 `assets\`，引用用相对路径，别用 `file:///` 或 `C:\` |
| 导入图片按钮没反应 | 确认装的是本工程打的包；系统选择器里要允许「文件」应用 |
| 导出的备份找不到 | Android 10+ 在 `内部存储/Download/学习备忘录/` |
| 构建报 `SDK location not found` | 检查 `local.properties` 的 `sdk.dir` |
| 构建报 Java 版本错误 | 必须 JDK 17，`java -version` 确认 |
| 想调试网页 | debug 包已开 WebView 调试，手机连电脑后 Chrome 打开 `chrome://inspect` |
