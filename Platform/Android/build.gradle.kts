plugins {
    id("com.android.application") version "8.4.0"
}

android {
    namespace = "org.main"
    compileSdk = 34

    defaultConfig {
        applicationId = "org.main"
        minSdk = 24
        targetSdk = 34
        versionCode = 1
        versionName = "1.0"
    }

    sourceSets {
        getByName("main") {
            java.srcDirs("java")
            manifest.srcFile("AndroidManifest.xml")
        }
    }
}