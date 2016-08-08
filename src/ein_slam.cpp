#include <iostream>
#include <stdio.h>
#include <random>
#include <math.h>
#include <chrono>
#include "opencv2/core/core_c.h"
#include "opencv2/core/core.hpp"
#include "opencv2/flann/miniflann.hpp"
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/video.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/ml/ml.hpp"
#include "opencv2/highgui/highgui_c.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/contrib/contrib.hpp"
#include "opencv2/nonfree/features2d.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include "ein_words.h"
#include "ein.h"

using namespace cv;
using namespace std;

vector<double> estimate_pos(Mat observed, Mat reconstructed) {
  double rot_max_val = 0;
  double rot_max_rot = 0;
  Point rot_max_loc;

  double width = min(observed.cols / 2, observed.rows / 2);

  Rect rect(width / 2, width / 2, width, width);
  Point2f reconstructed_center(reconstructed.cols/2.0F, reconstructed.rows/2.0F);
  Mat roi_observed;
  roi_observed = observed(rect);
  Mat max_rot;

  #pragma omp parallel for
  for(int angle = 0; angle < 360; angle += 1) {
    Mat rot_reconstructed;
    Mat result;
    double min_val; double max_val; Point min_loc; Point max_loc;

    /* Deal with rotation */
    Mat rot_mat = getRotationMatrix2D(reconstructed_center, angle, 1.0);
    warpAffine(reconstructed, rot_reconstructed, rot_mat, reconstructed.size());

    Mat rot_reconstructed_old = rot_reconstructed;
    matchTemplate(rot_reconstructed, roi_observed, result, CV_TM_CCOEFF_NORMED);

    minMaxLoc(result, &min_val, &max_val, &min_loc, &max_loc, Mat());


    if (max_val > rot_max_val) {
      rot_max_val = max_val;
      rot_max_rot = angle;
      rot_max_loc = max_loc;
      max_rot = rot_reconstructed;
    }
  }

  vector<double> to_return(5);
  to_return.insert(to_return.begin(), rot_max_loc.x - width + rect.x);
  to_return.insert(to_return.begin() + 1, rot_max_loc.x + width + rect.x);
  to_return.insert(to_return.begin() + 2, rot_max_loc.y - width + rect.y);
  to_return.insert(to_return.begin() + 3, rot_max_loc.y + width + rect.y);
  to_return.insert(to_return.begin() + 4, rot_max_rot);

  return to_return;
}

namespace ein_words {
  WORD(testSlam)
  virtual void execute(MachineState * ms) {
    Mat background;
    Mat observed;
    ms->config.scene->background_map.get()->rgbMuToMat(background);
    ms->config.scene->observed_map.get()->rgbMuToMat(observed);
    cvtColor(background, background, CV_YCrCb2BGR);
    cvtColor(observed, observed, CV_YCrCb2BGR);

    Rect roi = Rect(100, 100, 800, 800);

    vector<double> estimated = estimate_pos(observed(roi), background);

    double x_pos = (estimated[0] + estimated[1]) / 2 - 500;
    double y_pos = (estimated[2] + estimated[3]) / 2 - 500;

    cout << x_pos << ", " << y_pos << ", " << estimated[4] << endl;

    imshow("background", background);
    imshow("observed", observed(roi));
    waitKey(0);

    double border[] = {0, 0, 0, 0};
    if (estimated[1] > background.cols) {
      border[1] = estimated[1] - background.cols;
    }
    if (estimated[3] > background.rows) {
      border[3] = estimated[3] - background.rows;
    }

    Point2f center(background.cols/2.0F, background.rows/2.0F);
    Mat rot_mat = getRotationMatrix2D(center, estimated[4], 1.0);
    Mat rot_reconstructed;
    warpAffine(background, rot_reconstructed, rot_mat, background.size());

    cout << estimated[0] << ", " << estimated[1] << ", " << estimated[2] << ", " << estimated[3] << ", " << estimated[4] << endl;

    observed(roi).copyTo(rot_reconstructed.rowRange(estimated[2], estimated[3]).colRange(estimated[0], estimated[1]));

    Mat re_rot_mat = getRotationMatrix2D(center, -estimated[4], 1.0);
    warpAffine(rot_reconstructed, background, re_rot_mat, background.size());

    imshow("background", background);
    waitKey(0);
  }
  END_WORD
  REGISTER_WORD(testSlam)
}
