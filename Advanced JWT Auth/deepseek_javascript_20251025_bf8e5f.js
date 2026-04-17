class SecurityLogger {
  logAuthEvent(userId, event, ip, userAgent, metadata = {}) {
    const logEntry = {
      timestamp: new Date().toISOString(),
      userId,
      event,
      ip,
      userAgent,
      metadata
    };

    console.log('AUTH_EVENT:', JSON.stringify(logEntry));
    // In production, send to your logging system
  }

  logSuspiciousActivity(userId, activity, ip, details) {
    const logEntry = {
      timestamp: new Date().toISOString(),
      level: 'WARNING',
      userId,
      activity,
      ip,
      details
    };

    console.warn('SUSPICIOUS_ACTIVITY:', JSON.stringify(logEntry));
    // Alert security team in production
  }
}

const securityLogger = new SecurityLogger();