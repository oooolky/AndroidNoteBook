/**
 * ---------------------------------------------------------------
 * Author: Kaiyuan Lu
 * Project: Object Recognition System (CS5330 - Project 2)
 * Description:
 *   Real-time object recognition system using OpenCV.
 *   Includes thresholding, morphological filtering, segmentation,
 *   feature extraction, handcrafted feature database,
 *   and CNN-based one-shot classification using ResNet18.
 * ---------------------------------------------------------------
 */

#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <fstream>
using namespace cv;
using namespace std;

struct ObjectFeature {
    std::string label;
    double fillRatio;
    double aspectRatio;
    double hu0;
    double orientation;
};

// ===== Custom 3x3 Erosion/Dilation (white foreground=255, black background=0) =====
static inline uchar getPixel(const Mat& img, int y, int x) {
    return img.data[y * img.step + x];
}

Mat customErode3x3(const Mat& bin) {
    CV_Assert(bin.type() == CV_8UC1);
    Mat out = bin.clone();
    for (int y = 1; y < bin.rows - 1; ++y) {
        uchar* pOut = out.ptr<uchar>(y);
        for (int x = 1; x < bin.cols - 1; ++x) {
            // Erosion: keep white only if all neighbors are white
            bool allWhite = true;
            for (int dy = -1; dy <= 1 && allWhite; ++dy) {
                const uchar* pRow = bin.ptr<uchar>(y + dy);
                for (int dx = -1; dx <= 1; ++dx) {
                    if (pRow[x + dx] == 0) { allWhite = false; break; }
                }
            }
            pOut[x] = allWhite ? 255 : 0;
        }
    }
    return out;
}

Mat customDilate3x3(const Mat& bin) {
    CV_Assert(bin.type() == CV_8UC1);
    Mat out = bin.clone();
    for (int y = 1; y < bin.rows - 1; ++y) {
        uchar* pOut = out.ptr<uchar>(y);
        for (int x = 1; x < bin.cols - 1; ++x) {
            // Dilation: set white if any neighbor is white
            bool anyWhite = false;
            for (int dy = -1; dy <= 1 && !anyWhite; ++dy) {
                const uchar* pRow = bin.ptr<uchar>(y + dy);
                for (int dx = -1; dx <= 1; ++dx) {
                    if (pRow[x + dx] == 255) { anyWhite = true; break; }
                }
            }
            pOut[x] = anyWhite ? 255 : 0;
        }
    }
    return out;
}

// Custom Opening = Erode -> Dilate
Mat customOpening3x3(const Mat& bin) {
    Mat e = customErode3x3(bin);
    return customDilate3x3(e);
}

// Custom Closing = Dilate -> Erode
Mat customClosing3x3(const Mat& bin) {
    Mat d = customDilate3x3(bin);
    return customErode3x3(d);
}

// ===== Morphological cleanup using OpenCV (for faster tuning) =====
Mat cvOpening(const Mat& bin, int k = 3, int iters = 1) {
    Mat se = getStructuringElement(MORPH_ELLIPSE, Size(k, k));
    Mat out;
    morphologyEx(bin, out, MORPH_OPEN, se, Point(-1, -1), iters);
    return out;
}

Mat cvClosing(const Mat& bin, int k = 3, int iters = 1) {
    Mat se = getStructuringElement(MORPH_ELLIPSE, Size(k, k));
    Mat out;
    morphologyEx(bin, out, MORPH_CLOSE, se, Point(-1, -1), iters);
    return out;
}

// ===== Optional: remove small connected components =====
Mat removeSmallComponents(const Mat& bin, int minArea = 200) {
    Mat labels, stats, centroids;
    int n = connectedComponentsWithStats(bin, labels, stats, centroids, 8, CV_32S);
    Mat out = Mat::zeros(bin.size(), CV_8UC1);
    for (int i = 1; i < n; ++i) { // skip background (0)
        int area = stats.at<int>(i, CC_STAT_AREA);
        if (area >= minArea) {
            out.setTo(255, labels == i);
        }
    }
    return out;
}

// ===== Dynamic K-Means threshold (K=2) =====
int computeKMeansThreshold(const Mat& gray) {
    int sampleSize = gray.rows * gray.cols / 16;
    Mat samples(sampleSize, 1, CV_32F);
    RNG rng;

    for (int i = 0; i < sampleSize; i++) {
        int y = rng.uniform(0, gray.rows);
        int x = rng.uniform(0, gray.cols);
        samples.at<float>(i, 0) = (float)gray.at<uchar>(y, x);
    }

    Mat labels, centers;
    kmeans(samples, 2, labels,
        TermCriteria(TermCriteria::EPS + TermCriteria::COUNT, 10, 1.0),
        3, KMEANS_PP_CENTERS, centers);

    float t = (centers.at<float>(0, 0) + centers.at<float>(1, 0)) / 2.0f;
    return (int)t;
}

// ===== Load handcrafted feature database =====
vector<ObjectFeature> loadDatabase(const string& filename) {
    vector<ObjectFeature> db;
    ifstream fin(filename);
    if (!fin.is_open()) {
        cerr << "⚠️ Cannot open database file: " << filename << endl;
        return db;
    }

    string line;
    bool headerSkipped = false;
    while (getline(fin, line)) {
        if (!headerSkipped && line.find("label") != string::npos) {
            headerSkipped = true;
            continue;
        }

        stringstream ss(line);
        string label, s1, s2, s3, s4;
        if (!getline(ss, label, ',')) continue;
        if (!getline(ss, s1, ',')) continue;
        if (!getline(ss, s2, ',')) continue;
        if (!getline(ss, s3, ',')) continue;
        if (!getline(ss, s4, ',')) continue;

        try {
            ObjectFeature f;
            f.label = label;
            f.fillRatio = stod(s1);
            f.aspectRatio = stod(s2);
            f.hu0 = stod(s3);
            f.orientation = stod(s4);
            db.push_back(f);
        }
        catch (...) {
            cerr << "⚠️ Failed to parse: " << line << endl;
        }
    }

    cerr << "📚 Loaded handcrafted samples: " << db.size() << endl;
    return db;
}

// ===== Compute standard deviation for normalization =====
std::vector<double> computeStdDev(const std::vector<ObjectFeature>& db) {
    std::vector<double> stddev(4, 1.0);
    if (db.empty()) return stddev;

    std::vector<double> mean(4, 0.0);
    for (auto& f : db) {
        mean[0] += f.fillRatio;
        mean[1] += f.aspectRatio;
        mean[2] += f.hu0;
        mean[3] += f.orientation;
    }
    for (double& m : mean) m /= db.size();

    for (auto& f : db) {
        stddev[0] += pow(f.fillRatio - mean[0], 2);
        stddev[1] += pow(f.aspectRatio - mean[1], 2);
        stddev[2] += pow(f.hu0 - mean[2], 2);
        stddev[3] += pow(f.orientation - mean[3], 2);
    }
    for (double& s : stddev) s = sqrt(s / db.size());
    return stddev;
}

// ===== Nearest Neighbor Classification (handcrafted features) =====
std::string classifyObject(const ObjectFeature& test,
    const std::vector<ObjectFeature>& db,
    const std::vector<double>& stddev)
{
    if (db.empty()) return "No_DB";
    double minDist = 1e9;
    std::string bestLabel = "unknown";

    for (auto& f : db) {
        double d2 = pow((test.fillRatio - f.fillRatio) / stddev[0], 2)
            + pow((test.aspectRatio - f.aspectRatio) / stddev[1], 2)
            + pow((test.hu0 - f.hu0) / stddev[2], 2)
            + pow((test.orientation - f.orientation) / stddev[3], 2);
        double dist = sqrt(d2);
        if (dist < minDist) {
            minDist = dist;
            bestLabel = f.label;
        }
    }

    if (minDist > 5.0) bestLabel = "unknown";
    return bestLabel;
}

// ================================================================
// CNN One-Shot Embedding and Classification Utilities
// ================================================================

struct EmbSample { string label; Mat vec; };

// Save embedding to CSV
void appendEmbeddingCSV(const string& path, const string& label, const Mat& embedRow) {
    ofstream fout(path, ios::app);
    fout << label;
    for (int i = 0; i < embedRow.cols; i++) fout << "," << embedRow.at<float>(0, i);
    fout << "\n";
}

// Load embedding CSV
vector<EmbSample> loadEmbCSV(const string& path) {
    vector<EmbSample> db;
    ifstream fin(path);
    string line;
    while (getline(fin, line)) {
        stringstream ss(line); string token;
        if (!getline(ss, token, ',')) continue;
        EmbSample s; s.label = token;
        vector<float> v;
        while (getline(ss, token, ',')) v.push_back(stof(token));
        s.vec = Mat(1, (int)v.size(), CV_32F, v.data()).clone();
        db.push_back(std::move(s));
    }
    cerr << "📚 Loaded CNN embedding samples: " << db.size() << endl;
    return db;
}

// One-shot nearest neighbor classification using sum-squared difference
string classifyOneShotSSD(const Mat& q, const vector<EmbSample>& db, float unknownThresh = 35.0f) {
    if (db.empty()) return "No_Emb_DB";
    float best = 1e9f; string bestLabel = "unknown";
    for (auto& s : db) {
        Mat diff = q - s.vec; diff = diff.mul(diff);
        float d = sqrt((float)sum(diff)[0]);
        if (d < best) { best = d; bestLabel = s.label; }
    }
    if (best > unknownThresh) bestLabel = "unknown";
    return bestLabel;
}

// Load ResNet18 model
cv::dnn::Net loadResNet(const std::string& onnxPath) {
    cv::dnn::Net net = cv::dnn::readNetFromONNX(onnxPath);
    net.setPreferableBackend(cv::dnn::DNN_BACKEND_DEFAULT);
    net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
    cerr << "✅ Loaded ResNet18 model: " << onnxPath << endl;
    return net;
}

// Preprocess ROI (normalize, resize, blobify)
cv::Mat preprocess224(const cv::Mat& roi_bgr) {
    cv::Mat rgb; cv::cvtColor(roi_bgr, rgb, cv::COLOR_BGR2RGB);
    cv::Mat resized; cv::resize(rgb, resized, cv::Size(224, 224));
    cv::Mat blob = cv::dnn::blobFromImage(
        resized, 1.0 / 255.0, cv::Size(224, 224),
        cv::Scalar(0.485, 0.456, 0.406), true, false);
    return blob;
}

// Forward pass to get 512-D embedding
cv::Mat resnetEmbed(cv::dnn::Net& net, const cv::Mat& roi_bgr) {
    cv::Mat blob = preprocess224(roi_bgr);
    net.setInput(blob);
    cv::Mat feat = net.forward();
    cv::Mat flat = feat.reshape(1, 1);
    flat.convertTo(flat, CV_32F);
    return flat;
}

// Extract rotated and aligned ROI
cv::Mat extractAlignedROI(const cv::Mat& frame, const cv::Mat& regionMask,
    const cv::RotatedRect& rbox, double theta) {
    cv::Mat rotMat = cv::getRotationMatrix2D(rbox.center, -theta * 180.0 / CV_PI, 1.0);
    cv::Mat rotated;
    cv::warpAffine(frame, rotated, rotMat, frame.size(), cv::INTER_CUBIC);
    cv::Size rect_size = rbox.size;
    cv::Rect bbox(cvRound(rbox.center.x - rect_size.width / 2),
        cvRound(rbox.center.y - rect_size.height / 2),
        cvRound(rect_size.width),
        cvRound(rect_size.height));
    bbox &= cv::Rect(0, 0, rotated.cols, rotated.rows);
    cv::Mat roi = rotated(bbox).clone();
    return roi;
}

// ================================================================
// Main function (real-time system loop)
// ================================================================
int main() {
    // === Load video file ===
    string videoPath = "D:/code/LKY/VStudioRepo/ObjectRecognition/videos/testforpumpkin.mp4";
    VideoCapture cap(videoPath);
    auto database = loadDatabase("D:/code/LKY/VStudioRepo/ObjectRecognition/object_db.csv");
    auto stddevs = computeStdDev(database);
    string cnnLabel = "";

    if (!cap.isOpened()) {
        cerr << "❌ Cannot open video file: " << videoPath << endl;
        return -1;
    }

    cerr << "✅ Successfully opened video: " << videoPath << endl;
    cerr << "Press 'S' to save screenshots, 'ESC' to exit.\n";

    Mat frame, gray, blurred, binary;
    Mat regionMap1;
    int frameCount = 0, savedCount = 0;
    Mat lastEmb;

    while (true) {
        cap >> frame;
        if (frame.empty()) {
            cap.set(cv::CAP_PROP_POS_FRAMES, 0);
            continue;
        }

        // Preprocessing
        cvtColor(frame, gray, COLOR_BGR2GRAY);
        GaussianBlur(gray, blurred, Size(5, 5), 0);

        // ===== Task 1: Thresholding =====
        int T = computeKMeansThreshold(blurred);
        threshold(blurred, binary, T, 255, THRESH_BINARY_INV);

        // ===== Task 2: Morphological cleanup =====
        Mat cleaned = cvOpening(binary, 3, 1);
        cleaned = cvClosing(cleaned, 7, 2);
        cleaned = removeSmallComponents(cleaned, 200);

        // Fill holes (e.g., inside phones)
        Mat se = getStructuringElement(MORPH_RECT, Size(15, 15));
        morphologyEx(cleaned, cleaned, MORPH_CLOSE, se);
        Mat filled = cleaned.clone();
        cv::floodFill(filled, Point(0, 0), Scalar(255));
        bitwise_not(filled, filled);
        filled = (filled | cleaned);

        // Display early results
        imshow("Original", frame);
        imshow("Thresholded", binary);
        imshow("Cleaned", cleaned);
        imshow("Filled", filled);

        // ===========================================================
        // Task 3: Connected Components / Region Segmentation
        // ===========================================================
        Mat labels, stats, centroids;
        int numLabels = connectedComponentsWithStats(filled, labels, stats, centroids, 8, CV_32S);

        int minArea = 300;
        int maxRegions = 10;
        vector<pair<int, int>> valid;

        // Collect valid regions
        for (int i = 1; i < numLabels; ++i) {
            int area = stats.at<int>(i, CC_STAT_AREA);
            if (area >= minArea)
                valid.push_back({ i, area });
        }
        sort(valid.begin(), valid.end(),
            [](auto& a, auto& b) { return a.second > b.second; });
        if ((int)valid.size() > maxRegions)
            valid.resize(maxRegions);

        // Select one major object
        const int imgW = frame.cols, imgH = frame.rows;
        vector<pair<int, int>> cand = valid;
        const double maxAreaFrac = 0.95;
        const double maxAreaPix = maxAreaFrac * imgW * imgH;
        const int minAreaPix = 300;

        if (cand.empty()) {
            for (int i = 1; i < numLabels; ++i) {
                int area = stats.at<int>(i, CC_STAT_AREA);
                if (area >= minAreaPix) cand.push_back({ i, area });
            }
            sort(cand.begin(), cand.end(), [](auto& a, auto& b) { return a.second > b.second; });
        }

        int bestID = -1;
        double bestScore = -1.0;
        Point2d imgC(imgW * 0.5, imgH * 0.5);
        double maxCenterDist = hypot(imgC.x, imgC.y);

        for (auto& p : cand) {
            int i = p.first;
            int area = stats.at<int>(i, CC_STAT_AREA);
            if (area < minAreaPix || area > maxAreaPix) continue;

            double cx = centroids.at<double>(i, 0);
            double cy = centroids.at<double>(i, 1);
            double centerDist = hypot(cx - imgC.x, cy - imgC.y) / maxCenterDist;
            double centerScore = 1.0 - centerDist;
            double areaScore = (double)area / (imgW * imgH);
            double score = 0.8 * areaScore + 0.2 * centerScore;

            if (score > bestScore) { bestScore = score; bestID = i; }
        }

        if (bestID == -1 && !cand.empty()) bestID = cand.front().first;

        valid.clear();
        if (bestID != -1) {
            valid.push_back({ bestID, stats.at<int>(bestID, CC_STAT_AREA) });
        }

        // Color-coded region map
        vector<Vec3b> colors(numLabels, Vec3b(0, 0, 0));
        RNG rng(12345);
        for (auto& p : valid) {
            colors[p.first] = Vec3b(rng.uniform(50, 255),
                rng.uniform(50, 255),
                rng.uniform(50, 255));
        }

        Mat regionMap(labels.size(), CV_8UC3);
        for (int y = 0; y < labels.rows; ++y) {
            const int* row = labels.ptr<int>(y);
            Vec3b* out = regionMap.ptr<Vec3b>(y);
            for (int x = 0; x < labels.cols; ++x)
                out[x] = colors[row[x]];
        }

        // Draw region bounding box and center
        for (auto& p : valid) {
            int i = p.first;
            Rect box(stats.at<int>(i, CC_STAT_LEFT),
                stats.at<int>(i, CC_STAT_TOP),
                stats.at<int>(i, CC_STAT_WIDTH),
                stats.at<int>(i, CC_STAT_HEIGHT));
            Point2d c(centroids.at<double>(i, 0),
                centroids.at<double>(i, 1));
            rectangle(regionMap, box, Scalar(255, 255, 255), 1);
            circle(regionMap, c, 3, Scalar(255, 255, 255), -1);
        }

        regionMap1 = regionMap.clone();
        imshow("Region Map", regionMap1);

        // ===========================================================
        // Task 4: Feature Extraction and Classification
        // ===========================================================
        for (auto& p : valid) {
            int i = p.first;
            Mat regionMask = (labels == i);
            Moments mu = moments(regionMask, true);

            double cx = mu.m10 / mu.m00;
            double cy = mu.m01 / mu.m00;
            double theta = 0.5 * atan2(2 * mu.mu11, mu.mu20 - mu.mu02);
            double angle_deg = theta * 180 / CV_PI;

            vector<Point> pts;
            findNonZero(regionMask, pts);
            RotatedRect rbox = minAreaRect(pts);

            int area = stats.at<int>(i, CC_STAT_AREA);
            int bbox_w = stats.at<int>(i, CC_STAT_WIDTH);
            int bbox_h = stats.at<int>(i, CC_STAT_HEIGHT);
            double fillRatio = (double)area / (bbox_w * bbox_h);
            double aspectRatio = (double)bbox_h / bbox_w;

            // Draw oriented bounding box and major axis
            Point2f rectPts[4];
            rbox.points(rectPts);
            for (int j = 0; j < 4; ++j)
                line(regionMap, rectPts[j], rectPts[(j + 1) % 4], Scalar(255, 255, 255), 1);

            double len = 0.5 * std::max(bbox_w, bbox_h);
            Point2f pt1(cx - len * cos(theta), cy - len * sin(theta));
            Point2f pt2(cx + len * cos(theta), cy + len * sin(theta));
            line(regionMap, pt1, pt2, Scalar(0, 255, 255), 2);

            char buf[128];
            sprintf_s(buf, sizeof(buf), "ID:%d  Fill:%.2f  AR:%.2f", i, fillRatio, aspectRatio);
            putText(regionMap, buf, Point((int)cx - 40, (int)cy - 10),
                FONT_HERSHEY_SIMPLEX, 0.4, Scalar(255, 255, 255), 1);

            double hu[7];
            HuMoments(mu, hu);

            ObjectFeature test;
            test.fillRatio = fillRatio;
            test.aspectRatio = aspectRatio;
            test.hu0 = hu[0];
            test.orientation = angle_deg;

            string label = classifyObject(test, database, stddevs);

            Scalar color = (label == "unknown") ? Scalar(0, 0, 255) : Scalar(0, 255, 0);
            putText(regionMap, label, Point((int)cx - 30, (int)cy + 25),
                FONT_HERSHEY_SIMPLEX, 0.6, color, 2);

            // ===========================================================
            // Task 9: CNN-based one-shot classification (ResNet18)
            // ===========================================================
            static cv::dnn::Net cnnNet;
            static bool netLoaded = false;
            static vector<EmbSample> cnnDB;

            if (!netLoaded) {
                cnnNet = loadResNet("D:/code/LKY/VStudioRepo/ObjectRecognition/resnet18-v2-7.onnx");
                cnnDB = loadEmbCSV("cnn_db.csv");
                netLoaded = true;
            }

            Mat roi = extractAlignedROI(frame, regionMask, rbox, theta);
            lastEmb = resnetEmbed(cnnNet, roi);

            cnnLabel = classifyOneShotSSD(lastEmb, cnnDB, 55.0f);
            putText(regionMap, cnnLabel, Point((int)cx - 30, (int)cy + 45),
                FONT_HERSHEY_SIMPLEX, 0.6,
                cnnLabel == "unknown" ? Scalar(0, 0, 255) : Scalar(0, 255, 0), 2);
        }

        // Resize for display
        Mat resizedDisplay;
        resize(regionMap, resizedDisplay, Size(), 0.6, 0.6);
        imshow("Features", resizedDisplay);

        // ===========================================================
        // Keyboard controls
        // ===========================================================
        char key = (char)waitKey(20);
        if (key == 27) break;  // ESC = exit

        // Save handcrafted features
        if (key == 'n' || key == 'N') {
            if (valid.empty()) {
                std::cerr << "⚠️ No valid region detected.\n";
                continue;
            }
            int targetID = valid[0].first;
            Mat regionMask = (labels == targetID);
            Moments mu = moments(regionMask, true);
            double hu[7]; HuMoments(mu, hu);

            double cx = mu.m10 / mu.m00;
            double cy = mu.m01 / mu.m00;
            double theta = 0.5 * atan2(2 * mu.mu11, mu.mu20 - mu.mu02);
            double angle_deg = theta * 180 / CV_PI;

            int area = stats.at<int>(targetID, CC_STAT_AREA);
            int bbox_w = stats.at<int>(targetID, CC_STAT_WIDTH);
            int bbox_h = stats.at<int>(targetID, CC_STAT_HEIGHT);
            double fillRatio = (double)area / (bbox_w * bbox_h);
            double aspectRatio = (double)bbox_h / bbox_w;

            ObjectFeature f;
            std::cout << "\nEnter object label: ";
            std::cin >> f.label;

            f.fillRatio = fillRatio;
            f.aspectRatio = aspectRatio;
            f.hu0 = hu[0];
            f.orientation = angle_deg;

            std::ofstream fout("object_db.csv", std::ios::app);
            if (fout.is_open()) {
                fout << f.label << "," << f.fillRatio << "," << f.aspectRatio << ","
                    << f.hu0 << "," << f.orientation << "\n";
                fout.close();
                std::cout << "✅ Saved handcrafted feature: " << f.label << std::endl;
            }
            else {
                std::cerr << "❌ Failed to open object_db.csv for writing.\n";
            }
        }

        // Save CNN embedding
        if (key == 'e' || key == 'E') {
            string label;
            cout << "Enter object label: ";
            cin >> label;
            appendEmbeddingCSV("cnn_db.csv", label, lastEmb);
            cout << "💾 Saved CNN embedding sample: " << label << endl;
        }

        // Save screenshots
        if (key == 's' || key == 'S') {
            string index = to_string(savedCount++);
            string fnameBinary = "binary_" + index + ".png";
            string fnameCleaned = "cleaned_" + index + ".png";
            string fnameRegion1 = "regionmap1_" + index + ".png";
            string fnameRegion = "regionmap_" + index + ".png";
            string fnameDisplay = "features_" + index + ".png";

            imwrite(fnameBinary, binary);
            imwrite(fnameCleaned, cleaned);
            imwrite(fnameRegion, regionMap);
            imwrite(fnameDisplay, resizedDisplay);
            imwrite(fnameRegion1, regionMap1);
            cerr << "💾 Saved screenshots:\n"
                << " - " << fnameBinary << "\n"
                << " - " << fnameCleaned << "\n"
                << " - " << fnameRegion << "\n"
                << " - " << fnameDisplay << endl;
        }

        // Evaluation mode: record CNN predictions
        static std::ofstream evalLog("cnn_eval_results.csv", std::ios::app);
        static bool evalHeaderWritten = false;
        if (!evalHeaderWritten) {
            evalLog << "true_label,cnn_pred_label\n";
            evalHeaderWritten = true;
        }

        if (key == 'v' || key == 'V') {
            std::string trueLabel;
            std::cout << "\nEnter true label: ";
            std::cin >> trueLabel;

            evalLog << trueLabel << "," << cnnLabel << "\n";
            evalLog.flush();

            std::cout << "✅ Logged CNN evaluation → True: " << trueLabel
                << " | Predicted: " << cnnLabel << std::endl;
        }

        frameCount++;
    }

    cerr << "Video processing completed. Total frames: " << frameCount << endl;
    return 0;
}

