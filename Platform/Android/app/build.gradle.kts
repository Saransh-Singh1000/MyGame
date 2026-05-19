import java.io.File

plugins {
    id("com.android.application") version "8.5.2"
}

android {
    namespace = "org.main"
    compileSdk = 34

    ndkVersion = "26.3.11579264"

    defaultConfig {
        applicationId = "org.main"
        minSdk = 24
        targetSdk = 34
        versionCode = 1
        versionName = "1.0"
    }

    buildTypes {
        release {
            isMinifyEnabled = false
        }
        debug {
            isMinifyEnabled = false
        }
    }

    externalNativeBuild {
        cmake {
            path = file("../CMakeLists.txt")
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
}
