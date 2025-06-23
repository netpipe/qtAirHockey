#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QTimer>
#include <QSound>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QLabel>
#include <QPixmap>
#include <QKeyEvent>
#include <QVBoxLayout>
#include <cmath>

QPointF prevPlayerPaddle;
QPointF prevAiPaddle;


const int wallHeight = 20;
const int goalWidth = 30;
const int goalHeight = 100;

class AirHockey : public QWidget {
    QPointF puck, puckVel;
    QPointF playerPaddle, aiPaddle;
    float paddleRadius = 40, puckRadius = 15;
    QRectF table;
    QTimer *timer;
    QMediaPlayer *bgMusic;
    QSound *hitSound;
    int aiDifficulty = 2; // 0 = easy, 1 = medium, 2 = hard
public:
    AirHockey(QWidget *parent = nullptr) : QWidget(parent) {
        setFixedSize(800, 400);
        table = QRectF(0, 0, width(), height());
        puck = QPointF(width()/2, height()/2);
        puckVel = QPointF(4, 3);
        playerPaddle = QPointF(width()/4, height()/2);
        aiPaddle = QPointF(3*width()/4, height()/2);

        // Timer for game loop
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &AirHockey::updateGame);
        timer->start(16);

        // Background music
        QMediaPlaylist *playlist = new QMediaPlaylist();
        playlist->addMedia(QUrl("qrc:/music.wav"));
        playlist->setPlaybackMode(QMediaPlaylist::Loop);
        bgMusic = new QMediaPlayer(this);
        bgMusic->setPlaylist(playlist);
        bgMusic->setVolume(30);
        bgMusic->play();


        // Hit sound
        hitSound = new QSound(":/hit.wav", this);

        setMouseTracking(true);  // Allows mouseMoveEvent without clicking

    }

    void resetPuck() {
        puck = QPointF(width() / 2, height() / 2);
        puckVel = QPointF((qrand() % 2 ? 1 : -1) * 3, (qrand() % 5 - 2));
    }

    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.drawPixmap(rect(), QPixmap(":/table.png"));



        QPixmap wallTop(":/wall.png");
        QPixmap wallBottom(":/wall.png");
        QPixmap goalLeft(":/wall.png");
        QPixmap goalRight(":/wall.png");

        // Stretch top wall
        p.drawPixmap(QRectF(0, 0, width(), wallHeight), wallTop, QRectF(0, 0, wallTop.width(), wallTop.height()));
        // Stretch bottom wall
        p.drawPixmap(QRectF(0, height() - wallHeight, width(), wallHeight), wallBottom, QRectF(0, 0, wallBottom.width(), wallBottom.height()));
        // Stretch left goal
        p.drawPixmap(QRectF(0, (height() - goalHeight) / 2, goalWidth, goalHeight), goalLeft, QRectF(0, 0, goalLeft.width(), goalLeft.height()));
        // Stretch right goal
        p.drawPixmap(QRectF(width() - goalWidth, (height() - goalHeight) / 2, goalWidth, goalHeight), goalRight, QRectF(0, 0, goalRight.width(), goalRight.height()));

        QPixmap paddleTexture(":/paddle.png");
        QRectF targetRect(playerPaddle.x() - paddleRadius, playerPaddle.y() - paddleRadius,
                          2 * paddleRadius, 2 * paddleRadius);
        QRectF sourceRect(0, 0, paddleTexture.width(), paddleTexture.height());
        p.drawPixmap(targetRect, paddleTexture, sourceRect);

        targetRect = QRectF(aiPaddle.x() - paddleRadius, aiPaddle.y() - paddleRadius,
                            2 * paddleRadius, 2 * paddleRadius);
        p.drawPixmap(targetRect, paddleTexture, sourceRect);

        // Draw puck
        p.setBrush(Qt::black);
        p.drawEllipse(puck, puckRadius, puckRadius);

        // Draw paddles
        p.setBrush(Qt::blue);
        p.drawEllipse(playerPaddle, paddleRadius, paddleRadius);
        p.setBrush(Qt::red);
        p.drawEllipse(aiPaddle, paddleRadius, paddleRadius);
    }

    void mouseMoveEvent(QMouseEvent *e) override {
        QPointF newPos = e->pos();
        if (newPos.x() < width()/2) { // Restrict to left half
            playerPaddle = newPos;
            if (playerPaddle.y() < paddleRadius) playerPaddle.setY(paddleRadius);
            if (playerPaddle.y() > height() - paddleRadius) playerPaddle.setY(height() - paddleRadius);
        }
    }

    void keyPressEvent(QKeyEvent *e) override {
        if (e->key() == Qt::Key_1) aiDifficulty = 0;
        else if (e->key() == Qt::Key_2) aiDifficulty = 1;
        else if (e->key() == Qt::Key_3) aiDifficulty = 2;
    }

    void updateGame() {
        // Puck movement
        puck += puckVel;

            auto checkCollision = [&](QPointF paddlePos, QPointF paddleVel) {
                QPointF toPuck = puck - paddlePos;
                qreal dist = std::hypot(toPuck.x(), toPuck.y());

                if (dist < puckRadius + paddleRadius) {
                    QPointF normal = toPuck / dist;
                    puck = paddlePos + normal * (puckRadius + paddleRadius + 0.1); // separate

                    // Reflect puck velocity
                    qreal speedAlongNormal = QPointF::dotProduct(puckVel, normal);
                    QPointF impulse = normal * speedAlongNormal * 2.0;
                    puckVel -= impulse;

                    // Add only the paddle's push toward the puck
                    qreal pushPower = QPointF::dotProduct(paddleVel, normal);
                    if (pushPower > 0) {
                        puckVel += normal * pushPower * 0.5; // scale as needed
                    }

                    hitSound->play();
                }
            prevPlayerPaddle = playerPaddle;
            prevAiPaddle = aiPaddle;

        };
    QPointF playerVel = playerPaddle - prevPlayerPaddle;
    QPointF aiVel = aiPaddle - prevAiPaddle;
    checkCollision(playerPaddle, playerVel);
    checkCollision(aiPaddle, aiVel);
     //   checkCollision(playerPaddle);
     //   checkCollision(aiPaddle);

        // AI Movement
        float targetY = puck.y();
        float speed;
        switch (aiDifficulty) {
            case 0: speed = 1; break;
            case 1: speed = 3; break;
            case 2: speed = 5; break;
        }

        float dy = targetY - aiPaddle.y();
        if (std::abs(dy) > speed)
            aiPaddle.ry() += (dy > 0 ? speed : -speed);
        else
            aiPaddle.ry() += dy * 0.2; // Smooth final adjustment

        if (aiDifficulty == 2) {
            targetY += puckVel.y() * 10; // anticipate
        }

        // Clamp AI paddle
        if (aiPaddle.y() < paddleRadius) aiPaddle.setY(paddleRadius);
        if (aiPaddle.y() > height() - paddleRadius) aiPaddle.setY(height() - paddleRadius);

        // Wall collisions
        if (puck.y() <= puckRadius || puck.y() >= height() - puckRadius) {
            puckVel.setY(-puckVel.y());
            hitSound->play();
        }

        const qreal minWallBounceSpeed = 1.0; // tweakable

        // Top wall
        if (puck.y() - puckRadius < wallHeight) {
            puck.setY(wallHeight + puckRadius);
            if (std::abs(puckVel.y()) > minWallBounceSpeed) {
                puckVel.ry() *= -1;
                hitSound->play();
            }
        }

        // Bottom wall
        if (puck.y() + puckRadius > height() - wallHeight) {
            puck.setY(height() - wallHeight - puckRadius);
            if (std::abs(puckVel.y()) > minWallBounceSpeed) {
                puckVel.ry() *= -1;
                hitSound->play();
            }
        }


        // Define goal area range (shared for both goals)
        qreal goalYTop = (height() - goalHeight) / 2;
        qreal goalYBottom = goalYTop + goalHeight;
        bool inGoalY = puck.y() > goalYTop && puck.y() < goalYBottom;
        int wallThickness=1;
        // Left wall (outside goal area only)
        if (puck.x() - puckRadius < wallThickness && !inGoalY) {
            puck.setX(wallThickness + puckRadius);
            puckVel.rx() *= -1;
            hitSound->play();
        }

        // Right wall (outside goal area only)
        if (puck.x() + puckRadius > width() - wallThickness && !inGoalY) {
            puck.setX(width() - wallThickness - puckRadius);
            puckVel.rx() *= -1;
            hitSound->play();
        }

        // Left goal
        if (puck.x() - puckRadius < 0 && inGoalY) {
        //    aiScore++;
            resetPuck();
        }

        // Right goal
        if (puck.x() + puckRadius > width() && inGoalY) {
        //    playerScore++;
            resetPuck();
        }

        update();
    }
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    AirHockey w;
    w.setWindowTitle("Air Hockey Qt");
    w.show();
    return a.exec();
}
