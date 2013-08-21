#ifndef RATELIMITER_H
#define RATELIMITER_H

#include <QObject>

class RateLimiter : public QObject
{
    Q_OBJECT
public:
    explicit RateLimiter(QObject *parent = 0);
    
signals:
    
public slots:
    
};

#endif // RATELIMITER_H
