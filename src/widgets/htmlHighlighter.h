#pragma once

#include <QFont>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QVector>

// HTML syntax highlighter for use with the MOTD QPlainTextEdit.
// Colors are chosen to read well on both light and dark backgrounds.
class HtmlHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

    static constexpr QColor TAG_COLOR     { 0x56, 0x9c, 0xd6 }; // blue  — tag names & brackets
    static constexpr QColor VALUE_COLOR   { 0xce, 0x91, 0x78 }; // warm  — attribute values
    static constexpr QColor COMMENT_COLOR { 0x6a, 0x99, 0x55 }; // green — <!-- ... -->
    static constexpr QColor DOCTYPE_COLOR { 0x80, 0x80, 0x80 }; // gray  — <!DOCTYPE ...>

public:
    explicit HtmlHighlighter(QTextDocument* parent = nullptr) : QSyntaxHighlighter(parent)
    {
        // Tag names and closing brackets
        QTextCharFormat tagFmt;
        tagFmt.setForeground(TAG_COLOR);
        tagFmt.setFontWeight(QFont::Bold);
        addRule(QStringLiteral("</?[a-zA-Z][a-zA-Z0-9:-]*"), tagFmt); // <tag / </tag
        addRule(QStringLiteral("/>|>"),                        tagFmt); // > and />

        // Quoted attribute values  "..."  and  '...'
        QTextCharFormat valueFmt;
        valueFmt.setForeground(VALUE_COLOR);
        addRule(QStringLiteral("\"[^\"]*\""), valueFmt);
        addRule(QStringLiteral("'[^']*'"),   valueFmt);

        // DOCTYPE declaration
        QTextCharFormat doctypeFmt;
        doctypeFmt.setForeground(DOCTYPE_COLOR);
        doctypeFmt.setFontItalic(true);
        addRule(QStringLiteral("<!DOCTYPE[^>]*>"), doctypeFmt);

        // Comment format (applied via block-state logic below)
        m_commentFmt.setForeground(COMMENT_COLOR);
        m_commentFmt.setFontItalic(true);

        m_commentStart = QRegularExpression(QStringLiteral("<!--"));
        m_commentEnd   = QRegularExpression(QStringLiteral("-->"));
    }

protected:
    void highlightBlock(const QString& text) override
    {
        // Single-line rules
        for (const Rule& rule : m_rules)
        {
            QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
            while (it.hasNext())
            {
                const QRegularExpressionMatch m = it.next();
                setFormat(m.capturedStart(), m.capturedLength(), rule.format);
            }
        }

        // Multi-line comment block state: 0 = normal, 1 = inside comment
        setCurrentBlockState(0);

        int startIdx = (previousBlockState() == 1) ? 0 : text.indexOf(m_commentStart);

        while (startIdx >= 0)
        {
            const QRegularExpressionMatch endMatch = m_commentEnd.match(text, startIdx);
            int length = 0;
            if (endMatch.hasMatch())
            {
                length = endMatch.capturedStart() - startIdx + endMatch.capturedLength();
            }
            else
            {
                setCurrentBlockState(1);
                length = text.length() - startIdx;
            }
            setFormat(startIdx, length, m_commentFmt);
            startIdx = endMatch.hasMatch()
                ? text.indexOf(m_commentStart, startIdx + length)
                : -1;
        }
    }

private:
    struct Rule
    {
        QRegularExpression pattern;
        QTextCharFormat    format;
    };

    void addRule(const QString& patternStr, const QTextCharFormat& fmt)
    {
        m_rules.append({QRegularExpression(patternStr), fmt});
    }

    QVector<Rule>       m_rules;
    QTextCharFormat     m_commentFmt;
    QRegularExpression  m_commentStart;
    QRegularExpression  m_commentEnd;
};
