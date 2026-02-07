#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
CH+语言集成开发环境界面（PyQt6版本）
参照Visual Studio界面风格，使用PyQt6库实现
"""

from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QMenuBar, QMenu, QTreeWidget,
    QTreeWidgetItem, QTextEdit, QPlainTextEdit, QTextBrowser, QVBoxLayout, QHBoxLayout,
    QGridLayout, QWidget, QSplitter, QFileDialog, QMessageBox, QInputDialog, QLineEdit,
    QCheckBox, QComboBox, QLabel, QFrame, QPushButton, QDialog, QGroupBox,
    QRadioButton, QColorDialog, QListWidget, QListWidgetItem, QSpinBox,
    QDoubleSpinBox, QStackedWidget, QScrollArea
)
from PyQt6.QtCore import Qt, QSize, QRegularExpression, QSettings, pyqtSignal, QObject, QTimer, QRect
from PyQt6.QtGui import (
    QSyntaxHighlighter, QTextCharFormat, QColor, QFont, QKeySequence, QAction, QPainter, QTextBlock, QPalette, QTextCursor, QIcon
)
import sys
import os
import subprocess
import threading
import configparser
import queue
import re

# Windows平台导入CREATE_NEW_CONSOLE标志
if sys.platform == "win32":
    import subprocess
    CREATE_NEW_CONSOLE = subprocess.CREATE_NEW_CONSOLE

class LineNumberArea(QWidget):
    """
    @class LineNumberArea
    @brief 行号区域组件
    
    负责显示文本编辑器的行号，通过缓存机制提高绘制性能
    """
    
    def __init__(self, editor):
        """
        @brief 构造函数
        @param editor 关联的文本编辑器
        """
        super(LineNumberArea, self).__init__(editor)
        self.editor = editor
        self.cached_line_count = 0  # 缓存行数
        self.cached_width = 0       # 缓存宽度
    
    def sizeHint(self):
        """
        @brief 返回推荐大小
        @return 推荐的大小
        
        性能优化：缓存行号区域宽度，避免重复计算
        """
        current_line_count = self.editor.blockCount()
        if current_line_count != self.cached_line_count:
            self.cached_width = self.editor.line_number_area_width()
            self.cached_line_count = current_line_count
        return QSize(self.cached_width, 0)
    
    def paintEvent(self, event):
        """
        @brief 绘制行号
        @param event 绘制事件
        
        性能优化：委托给编辑器进行绘制，保持绘制逻辑集中
        """
        self.editor.line_number_area_paint_event(event)

class CHPlusTextEdit(QPlainTextEdit):
    """
    @class CHPlusTextEdit
    @brief 支持实时中文符号转换和行号显示的文本编辑器
    
    性能优化：缓存行号计算、优化高亮算法、减少不必要的重绘
    """
    
    def __init__(self, parent=None):
        """
        @brief 构造函数
        @param parent 父组件
        
        性能优化：延迟初始化、缓存计算结果、优化信号连接
        """
        super(CHPlusTextEdit, self).__init__(parent)
        self.parent_ide = parent
        self.line_number_area = LineNumberArea(self)
        
        # 性能优化：缓存变量
        self.cached_line_count = 0
        self.cached_line_number_width = 0
        self.last_cursor_position = -1
        
        # 连接信号（性能优化：使用直接连接减少信号传递开销）
        self.blockCountChanged.connect(self.update_line_number_area_width, 
                                      Qt.ConnectionType.DirectConnection)
        self.updateRequest.connect(self.update_line_number_area, 
                                  Qt.ConnectionType.DirectConnection)
        self.cursorPositionChanged.connect(self.highlight_current_line, 
                                          Qt.ConnectionType.DirectConnection)
        
        # 初始化行号区域宽度
        # 强制设置最小边距，避免符号被遮挡
        self.setViewportMargins(20, 0, 0, 0)
        
        # 延迟高亮当前行，避免空文档时的问题
        QTimer.singleShot(100, self.highlight_current_line)
    
    def line_number_area_width(self):
        """
        @brief 计算行号区域宽度
        @return 行号区域宽度
        
        性能优化：缓存计算结果，避免重复计算
        """
        current_block_count = self.blockCount()
        if current_block_count == self.cached_line_count and self.cached_line_number_width > 0:
            return self.cached_line_number_width
        
        # 确保至少显示1位数字，即使文档为空
        digits = 1
        max_value = max(1, current_block_count)
        while max_value >= 10:
            max_value //= 10  # 性能优化：使用整数除法
            digits += 1
        
        # 性能优化：缓存字体度量计算
        digit_width = self.fontMetrics().horizontalAdvance('9')
        # 增加边距，确保符号也能正常显示
        space = 8 + digit_width * digits
        
        # 更新缓存
        self.cached_line_count = current_block_count
        self.cached_line_number_width = space
        
        return space
    
    def update_line_number_area_width(self, new_block_count):
        """
        @brief 更新行号区域宽度
        @param new_block_count 新的块数量
        
        性能优化：只在行数变化时更新
        """
        if new_block_count != self.cached_line_count:
            # 确保最小边距，避免符号被遮挡
            min_margin = 20  # 最小边距，确保符号能正常显示
            calculated_width = self.line_number_area_width()
            actual_width = max(min_margin, calculated_width)
            self.setViewportMargins(actual_width, 0, 0, 0)
    
    def update_line_number_area(self, rect, dy):
        """
        @brief 更新行号区域
        @param rect 更新区域
        @param dy 垂直偏移量
        
        性能优化：减少不必要的更新，只更新可见区域
        """
        if dy:
            self.line_number_area.scroll(0, dy)
        else:
            # 性能优化：只更新需要重绘的区域
            update_rect = QRect(0, rect.y(), self.line_number_area.width(), rect.height())
            self.line_number_area.update(update_rect)
        
        # 性能优化：只在需要时更新宽度
        if rect.contains(self.viewport().rect()):
            self.update_line_number_area_width(0)
    
    def resizeEvent(self, event):
        """重写大小改变事件"""
        super().resizeEvent(event)
        cr = self.contentsRect()
        self.line_number_area.setGeometry(QRect(cr.left(), cr.top(), self.line_number_area_width(), cr.height()))
    
    def line_number_area_paint_event(self, event):
        """
        @brief 绘制行号
        @param event 绘制事件
        
        性能优化：减少绘制调用、缓存字体度量、优化循环
        """
        painter = QPainter(self.line_number_area)
        
        # 性能优化：缓存背景色和字体度量
        if not hasattr(self, '_cached_bg_color'):
            self._cached_bg_color = QColor("#F0F0F0")
            self._cached_text_color = QColor("#666666")
            self._cached_font_height = self.fontMetrics().height()
        
        # 填充背景
        painter.fillRect(event.rect(), self._cached_bg_color)
        
        # 性能优化：只绘制可见区域的行号
        block = self.firstVisibleBlock()
        block_number = block.blockNumber()
        top = self.blockBoundingGeometry(block).translated(self.contentOffset()).top()
        bottom = top + self.blockBoundingRect(block).height()
        
        # 设置画笔颜色
        painter.setPen(self._cached_text_color)
        
        # 性能优化：预计算区域边界
        event_top = event.rect().top()
        event_bottom = event.rect().bottom()
        line_number_width = self.line_number_area.width()
        
        while block.isValid() and top <= event_bottom:
            if block.isVisible() and bottom >= event_top:
                # 性能优化：避免字符串重复创建
                number = str(block_number + 1)
                painter.drawText(0, int(top), line_number_width, self._cached_font_height,
                                Qt.AlignmentFlag.AlignCenter, number)
            
            # 移动到下一个块
            block = block.next()
            if not block.isValid():
                break
                
            top = bottom
            bottom = top + self.blockBoundingRect(block).height()
            block_number += 1
    
    def highlight_current_line(self):
        """高亮当前行和匹配的括号"""
        extra_selections = []
        
        if not self.isReadOnly():
            # 高亮当前行
            selection = QTextEdit.ExtraSelection()
            line_color = QColor(Qt.GlobalColor.yellow).lighter(160)
            selection.format.setBackground(line_color)
            selection.format.setProperty(QTextCharFormat.Property.FullWidthSelection, True)
            selection.cursor = self.textCursor()
            selection.cursor.clearSelection()
            extra_selections.append(selection)
            
            # 高亮匹配的括号（如果启用）
            if hasattr(self.parent_ide, 'settings') and self.parent_ide.settings.get("highlight_bracket", True):
                bracket_selections = self.highlight_matching_brackets()
                extra_selections.extend(bracket_selections)
        
        self.setExtraSelections(extra_selections)
    
    def highlight_matching_brackets(self):
        """高亮匹配的括号"""
        selections = []
        cursor = self.textCursor()
        
        # 如果光标没有选择任何文本，检查当前位置的字符
        if cursor.position() > 0 and not cursor.hasSelection():
            # 获取光标位置
            pos = cursor.position()
            text_length = len(self.toPlainText())
            
            # 定义括号对
            bracket_pairs = {
                '(': ')',
                ')': '(',
                '[': ']',
                ']': '[',
                '{': '}',
                '}': '{'
            }
            
            # 检查光标位置是否在括号上
            current_char = None
            is_left = False
            bracket_pos = -1
            
            # 检查光标左侧的字符
            if pos > 0:
                cursor_left = QTextCursor(cursor)
                cursor_left.setPosition(pos - 1)
                cursor_left.setPosition(pos, QTextCursor.MoveMode.KeepAnchor)
                char_left = cursor_left.selectedText()
                if char_left in bracket_pairs:
                    current_char = char_left
                    is_left = char_left in '([{'
                    bracket_pos = pos - 1
            
            # 检查光标右侧的字符（如果左侧没有找到括号）
            if current_char is None and pos < text_length:
                cursor_right = QTextCursor(cursor)
                cursor_right.setPosition(pos)
                cursor_right.setPosition(pos + 1, QTextCursor.MoveMode.KeepAnchor)
                char_right = cursor_right.selectedText()
                if char_right in bracket_pairs:
                    current_char = char_right
                    is_left = char_right in '([{'
                    bracket_pos = pos
            
            if current_char and bracket_pos != -1:
                # 找到匹配的括号
                match_pos = self.find_matching_bracket(bracket_pos, current_char, is_left)
                if match_pos != -1 and match_pos < text_length:
                    # 高亮当前括号
                    selection1 = QTextEdit.ExtraSelection()
                    selection1.format.setBackground(QColor(173, 216, 230))  # 浅蓝色
                    selection1.format.setForeground(QColor(0, 0, 0))
                    cursor1 = QTextCursor(cursor)
                    cursor1.setPosition(bracket_pos)
                    cursor1.setPosition(bracket_pos + 1, QTextCursor.MoveMode.KeepAnchor)
                    selection1.cursor = cursor1
                    selections.append(selection1)
                    
                    # 高亮匹配的括号
                    selection2 = QTextEdit.ExtraSelection()
                    selection2.format.setBackground(QColor(173, 216, 230))  # 浅蓝色
                    selection2.format.setForeground(QColor(0, 0, 0))
                    cursor2 = QTextCursor(cursor)
                    cursor2.setPosition(match_pos)
                    cursor2.setPosition(match_pos + 1, QTextCursor.MoveMode.KeepAnchor)
                    selection2.cursor = cursor2
                    selections.append(selection2)
        
        return selections
    
    def find_matching_bracket(self, pos, bracket, is_left):
        """找到匹配的括号位置"""
        # 定义括号对
        bracket_pairs = {
            '(': ')',
            ')': '(',
            '[': ']',
            ']': '[',
            '{': '}',
            '}': '{'
        }
        
        target_bracket = bracket_pairs[bracket]
        direction = 1 if is_left else -1
        
        # 获取文档文本
        document = self.document()
        text = document.toPlainText()
        
        # 从当前位置开始搜索
        current_pos = pos + (1 if is_left else -1)
        bracket_count = 0
        
        while 0 <= current_pos < len(text):
            char = text[current_pos]
            
            # 跳过字符串和注释
            if self.is_in_string_or_comment(current_pos):
                current_pos += direction
                continue
            
            # 如果是当前类型的括号，增加计数
            if char == bracket:
                bracket_count += 1
            # 如果是目标类型的括号
            elif char == target_bracket:
                if bracket_count == 0:
                    return current_pos  # 找到匹配的括号
                else:
                    bracket_count -= 1
            
            current_pos += direction
        
        return -1  # 没有找到匹配的括号
    
    def is_in_string_or_comment(self, pos):
        """检查位置是否在字符串或注释中"""
        document = self.document()
        cursor = QTextCursor(document)
        cursor.setPosition(pos)
        
        # 获取当前位置的块格式
        block = cursor.block()
        block_text = block.text()
        
        # 检查是否在注释中
        if '#' in block_text:
            comment_pos = block_text.find('#')
            if comment_pos <= cursor.positionInBlock():
                return True
        
        # 检查是否在字符串中（简化实现）
        # 这里可以更精确地实现字符串检测
        return False
    
    def keyPressEvent(self, event):
        """重写按键事件，实现回车时转换当前行的中文符号"""
        # 检测回车键
        if event.key() in [Qt.Key.Key_Return, Qt.Key.Key_Enter] and self.parent_ide and self.parent_ide.convert_chinese_to_english:
            # 获取当前光标位置
            cursor = self.textCursor()
            current_line = cursor.blockNumber()
            
            # 获取当前行的文本
            block = cursor.block()
            line_text = block.text()
            
            # 对当前行进行中文符号转换
            converted_line = self.parent_ide.convert_chinese_to_english_text(line_text)
            
            # 如果有转换，替换当前行
            if converted_line != line_text:
                # 选中当前行
                cursor.select(QTextCursor.SelectionType.LineUnderCursor)
                # 替换为转换后的文本
                cursor.insertText(converted_line)
                # 移动光标到行尾
                cursor.movePosition(QTextCursor.MoveOperation.EndOfLine)
                self.setTextCursor(cursor)
            
            # 插入换行
            super().keyPressEvent(event)
        else:
            # 正常处理按键
            super().keyPressEvent(event)
    
    def is_cursor_in_string(self):
        """检查光标是否在字符串中"""
        cursor = self.textCursor()
        position = cursor.position()
        
        # 获取文档文本
        document = self.document()
        text = document.toPlainText()
        
        # 检查光标位置是否在字符串中
        in_string = False
        string_char = None
        i = 0
        
        while i < position:
            char = text[i]
            
            if not in_string and char in '"\'':
                in_string = True
                string_char = char
            elif in_string and char == string_char:
                # 检查是否是转义字符
                if i > 0 and text[i-1] == '\\':
                    pass
                else:
                    in_string = False
                    string_char = None
            
            i += 1
        
        return in_string
    
    def wheelEvent(self, event):
        """重写滚轮事件，支持Ctrl+滚轮调整字体大小"""
        if event.modifiers() & Qt.KeyboardModifier.ControlModifier:
            # 获取当前字体
            font = self.font()
            current_size = font.pointSize()
            
            # 根据滚轮方向调整字体大小
            delta = event.angleDelta().y()
            if delta > 0:
                # 向上滚动，增大字体
                new_size = min(current_size + 1, 72)
            else:
                # 向下滚动，减小字体
                new_size = max(current_size - 1, 6)
            
            # 应用新字体大小
            font.setPointSize(new_size)
            self.setFont(font)
            
            # 阻止默认滚动行为
            event.accept()
        else:
            # 正常滚动
            super().wheelEvent(event)

class ModernHelpDialog(QDialog):
    """
    @class ModernHelpDialog
    @brief 现代化的Markdown帮助对话框
    
    提供代码栏一键复制、向下滑动、语法高亮等功能
    """
    
    def __init__(self, parent=None, markdown_content=""):
        """
        @brief 构造函数
        @param parent 父组件
        @param markdown_content Markdown内容
        """
        super(ModernHelpDialog, self).__init__(parent)
        self.markdown_content = markdown_content
        self.setup_ui()
        self.setup_styles()
        self.process_markdown()
    
    def setup_ui(self):
        """设置用户界面"""
        # 主布局
        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.setSpacing(0)
        
        # 标题栏
        self.setup_title_bar(main_layout)
        
        # 内容区域
        self.setup_content_area(main_layout)
        
        # 底部工具栏
        self.setup_toolbar(main_layout)
    
    def setup_title_bar(self, layout):
        """设置标题栏"""
        title_bar = QWidget()
        title_bar.setFixedHeight(40)
        title_bar.setStyleSheet("""
            background-color: #2c3e50;
            color: white;
            font-size: 14px;
            font-weight: bold;
            padding: 0 15px;
        """)
        
        title_layout = QHBoxLayout(title_bar)
        title_layout.setContentsMargins(0, 0, 0, 0)
        
        title_label = QLabel("CH+ 中文编辑器 - 帮助文档")
        title_label.setStyleSheet("color: white; font-size: 14px;")
        
        close_button = QPushButton("×")
        close_button.setFixedSize(30, 30)
        close_button.setStyleSheet("""
            QPushButton {
                background-color: transparent;
                color: white;
                border: none;
                font-size: 18px;
                font-weight: bold;
                outline: none;
            }
            QPushButton:hover {
                background-color: #e74c3c;
                border-radius: 3px;
            }
            QPushButton:focus {
                background-color: transparent;
                color: white;
                border: none;
                outline: none;
            }
        """)
        close_button.clicked.connect(self.close)
        
        title_layout.addWidget(title_label)
        title_layout.addStretch()
        title_layout.addWidget(close_button)
        
        layout.addWidget(title_bar)
    
    def setup_content_area(self, layout):
        """设置内容区域"""
        # 创建分割器
        splitter = QSplitter(Qt.Orientation.Horizontal)
        
        # 左侧目录
        self.setup_toc(splitter)
        
        # 右侧内容
        self.setup_content(splitter)
        
        # 设置分割比例
        splitter.setSizes([200, 800])
        layout.addWidget(splitter)
    
    def setup_toc(self, splitter):
        """设置目录区域"""
        toc_widget = QWidget()
        toc_layout = QVBoxLayout(toc_widget)
        
        # 目录标题
        toc_label = QLabel("目录")
        toc_label.setStyleSheet("""
            font-size: 16px;
            font-weight: bold;
            padding: 10px;
            background-color: #f8f9fa;
            border-bottom: 1px solid #dee2e6;
        """)
        
        # 目录列表
        self.toc_list = QListWidget()
        self.toc_list.setStyleSheet("""
            QListWidget {
                border: none;
                background-color: white;
            }
            QListWidget::item {
                padding: 8px 15px;
                border-bottom: 1px solid #f0f0f0;
            }
            QListWidget::item:selected {
                background-color: #3498db;
                color: white;
            }
            QListWidget::item:hover {
                background-color: #ecf0f1;
            }
        """)
        
        # 连接目录点击事件
        self.toc_list.itemClicked.connect(self.on_toc_item_clicked)
        
        toc_layout.addWidget(toc_label)
        toc_layout.addWidget(self.toc_list)
        
        splitter.addWidget(toc_widget)
    
    def setup_content(self, splitter):
        """设置内容区域"""
        content_widget = QWidget()
        content_layout = QVBoxLayout(content_widget)
        
        # 创建滚动区域
        scroll_area = QScrollArea()
        scroll_area.setWidgetResizable(True)
        scroll_area.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
        scroll_area.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
        
        # 内容容器
        self.content_container = QWidget()
        self.content_layout = QVBoxLayout(self.content_container)
        self.content_layout.setAlignment(Qt.AlignmentFlag.AlignTop)
        
        scroll_area.setWidget(self.content_container)
        content_layout.addWidget(scroll_area)
        
        splitter.addWidget(content_widget)
    
    def setup_toolbar(self, layout):
        """设置底部工具栏"""
        toolbar = QWidget()
        toolbar.setFixedHeight(40)
        toolbar.setStyleSheet("""
            background-color: #ecf0f1;
            border-top: 1px solid #bdc3c7;
        """)
        
        toolbar_layout = QHBoxLayout(toolbar)
        toolbar_layout.setContentsMargins(15, 5, 15, 5)
        
        # 复制按钮
        copy_button = QPushButton("复制全部代码")
        copy_button.setStyleSheet("""
            QPushButton {
                background-color: #3498db;
                color: white;
                border: none;
                padding: 5px 15px;
                border-radius: 3px;
                font-size: 12px;
                outline: none;
            }
            QPushButton:hover {
                background-color: #2980b9;
            }
            QPushButton:focus {
                background-color: #3498db;
                color: white;
                border: none;
                outline: none;
            }
        """)
        copy_button.clicked.connect(self.copy_all_code)
        
        # 搜索框
        self.search_edit = QLineEdit()
        self.search_edit.setPlaceholderText("搜索帮助内容...")
        self.search_edit.setStyleSheet("""
            QLineEdit {
                padding: 5px 10px;
                border: 1px solid #bdc3c7;
                border-radius: 3px;
                font-size: 12px;
                outline: none;
            }
            QLineEdit:focus {
                padding: 5px 10px;
                border: 1px solid #bdc3c7;
                border-radius: 3px;
                font-size: 12px;
                outline: none;
            }
        """)
        self.search_edit.textChanged.connect(self.on_search_text_changed)
        
        toolbar_layout.addWidget(copy_button)
        toolbar_layout.addStretch()
        toolbar_layout.addWidget(QLabel("搜索:"))
        toolbar_layout.addWidget(self.search_edit)
        
        layout.addWidget(toolbar)
    
    def setup_styles(self):
        """设置样式"""
        self.setStyleSheet("""
            QDialog {
                background-color: white;
            }
        """)
    
    def process_markdown(self):
        """处理Markdown内容并显示"""
        # 清空现有内容
        for i in reversed(range(self.content_layout.count())):
            widget = self.content_layout.itemAt(i).widget()
            if widget:
                widget.setParent(None)
        
        # 清空目录
        self.toc_list.clear()
        
        # 解析Markdown并创建内容
        lines = self.markdown_content.split('\n')
        current_section = ""
        in_code_block = False
        
        for line in lines:
            if line.strip().startswith('```'):
                # 代码块开始/结束
                in_code_block = not in_code_block
                if not in_code_block:
                    # 代码块结束，重置当前代码块
                    if hasattr(self, 'current_code_block'):
                        delattr(self, 'current_code_block')
                        delattr(self, 'current_code_text')
                continue
            
            if in_code_block:
                # 代码行
                self.add_code_line(line)
            elif line.startswith('#'):
                # 标题
                level = len(line) - len(line.lstrip('#'))
                title_text = line.lstrip('#').strip()
                self.add_title(title_text, level)
                
                # 添加到目录
                if level <= 2:  # 只添加一级和二级标题到目录
                    self.add_to_toc(title_text, level)
                    
            elif line.strip():
                # 普通文本
                self.add_text_line(line)
            else:
                # 空行
                self.add_spacing()
    
    def add_title(self, text, level):
        """添加标题"""
        title_label = QLabel(text)
        
        if level == 1:
            title_label.setStyleSheet("""
                font-size: 24px;
                font-weight: bold;
                color: #2c3e50;
                margin: 20px 0 10px 0;
                padding: 10px 0;
                border-bottom: 2px solid #3498db;
            """)
        elif level == 2:
            title_label.setStyleSheet("""
                font-size: 20px;
                font-weight: bold;
                color: #34495e;
                margin: 15px 0 8px 0;
                padding: 8px 0;
                border-bottom: 1px solid #bdc3c7;
            """)
        else:
            title_label.setStyleSheet("""
                font-size: 16px;
                font-weight: bold;
                color: #7f8c8d;
                margin: 10px 0 5px 0;
            """)
        
        self.content_layout.addWidget(title_label)
    
    def add_text_line(self, text):
        """添加文本行"""
        if not text.strip():
            return
            
        text_label = QLabel(text.strip())
        text_label.setStyleSheet("""
            font-size: 14px;
            color: #2c3e50;
            margin: 5px 0;
            line-height: 1.5;
        """)
        text_label.setWordWrap(True)
        self.content_layout.addWidget(text_label)
    
    def add_code_line(self, code):
        """添加代码行"""
        if not hasattr(self, 'current_code_block'):
            # 创建代码块容器
            self.current_code_block = QWidget()
            code_layout = QVBoxLayout(self.current_code_block)
            code_layout.setContentsMargins(10, 5, 10, 5)
            code_layout.setSpacing(0)
            
            # 代码文本框
            self.current_code_text = ""
            code_text = QTextEdit()
            
            # 代码块标题栏
            code_header = QWidget()
            header_layout = QHBoxLayout(code_header)
            header_layout.setContentsMargins(10, 5, 10, 5)
            
            language_label = QLabel("CH+")
            language_label.setStyleSheet("""
                font-size: 12px;
                color: #7f8c8d;
                font-weight: bold;
            """)
            
            copy_button = QPushButton("复制")
            copy_button.setFixedSize(50, 25)
            copy_button.setStyleSheet("""
                QPushButton {
                    background-color: #27ae60;
                    color: white;
                    border: none;
                    border-radius: 3px;
                    font-size: 10px;
                    outline: none;
                }
                QPushButton:hover {
                    background-color: #229954;
                }
                QPushButton:focus {
                    background-color: #27ae60;
                    color: white;
                    border: none;
                    outline: none;
                }
            """)
            # 修复：让复制按钮直接从代码文本框中获取文本
            copy_button.clicked.connect(lambda: self.copy_code(code_text.toPlainText()))
            
            header_layout.addWidget(language_label)
            header_layout.addStretch()
            header_layout.addWidget(copy_button)
            code_text.setFont(QFont("Consolas", 11))
            code_text.setStyleSheet("""
                QTextEdit {
                    background-color: #f8f9fa;
                    border: 1px solid #dee2e6;
                    border-radius: 0 0 5px 5px;
                    padding: 10px;
                    color: #2c3e50;
                }
            """)
            code_text.setReadOnly(True)
            
            code_layout.addWidget(code_header)
            code_layout.addWidget(code_text)
            
            self.content_layout.addWidget(self.current_code_block)
        
        # 添加代码行
        self.current_code_text += code + '\n'
        code_text_widget = self.current_code_block.layout().itemAt(1).widget()
        code_text_widget.setPlainText(self.current_code_text)
    
    def add_spacing(self):
        """添加间距"""
        spacer = QWidget()
        spacer.setFixedHeight(10)
        self.content_layout.addWidget(spacer)
    
    def add_to_toc(self, title, level):
        """添加到目录"""
        item = QListWidgetItem(title)
        if level == 1:
            item.setFont(QFont("微软雅黑", 12, QFont.Weight.Bold))
        else:
            item.setFont(QFont("微软雅黑", 10))
        
        self.toc_list.addItem(item)
    
    def copy_code(self, code):
        """复制代码"""
        clipboard = QApplication.clipboard()
        clipboard.setText(code.strip())
        # 显示复制成功提示（使用无边框样式）
        msg_box = QMessageBox(self)
        msg_box.setText("复制成功！")
        msg_box.setStyleSheet("""
            QMessageBox {
                background-color: #ffffff;
                border: none;
                padding: 10px;
            }
            QLabel {
                color: #2c3e50;
                font-size: 14px;
            }
            QPushButton {
                background-color: #3498db;
                color: white;
                border: none;
                padding: 5px 15px;
                border-radius: 3px;
                min-width: 80px;
                outline: none;
            }
            QPushButton:hover {
                background-color: #2980b9;
            }
        """)
        msg_box.exec()
    
    def copy_all_code(self):
        """复制全部代码"""
        # 提取所有代码块内容
        all_code = ""
        for i in range(self.content_layout.count()):
            widget = self.content_layout.itemAt(i).widget()
            if hasattr(widget, 'layout') and widget.layout():
                # 检查是否是代码块
                if widget.layout().count() >= 2:
                    code_text_widget = widget.layout().itemAt(1).widget()
                    if isinstance(code_text_widget, QTextEdit):
                        all_code += code_text_widget.toPlainText() + '\n\n'
        
        if all_code:
            clipboard = QApplication.clipboard()
            clipboard.setText(all_code.strip())
            # 显示复制成功提示（使用无边框样式）
            msg_box = QMessageBox(self)
            msg_box.setText("所有代码复制成功！")
            msg_box.setStyleSheet("""
                QMessageBox {
                    background-color: #ffffff;
                    border: none;
                    padding: 10px;
                }
                QLabel {
                    color: #2c3e50;
                    font-size: 14px;
                }
                QPushButton {
                    background-color: #3498db;
                    color: white;
                    border: none;
                    padding: 5px 15px;
                    border-radius: 3px;
                    min-width: 80px;
                    outline: none;
                }
                QPushButton:hover {
                    background-color: #2980b9;
                }
            """)
            msg_box.exec()
        else:
            # 静默处理，不显示提示
            pass
    
    def on_toc_item_clicked(self, item):
        """目录项点击事件 - 跳转到对应标题"""
        title_text = item.text()
        
        # 查找对应的标题控件
        for i in range(self.content_layout.count()):
            widget = self.content_layout.itemAt(i).widget()
            if isinstance(widget, QLabel) and widget.text() == title_text:
                # 滚动到该标题
                scroll_area = self.findChild(QScrollArea)
                if scroll_area:
                    # 计算标题位置并滚动
                    scroll_area.ensureWidgetVisible(widget)
                    
                    # 高亮显示目标标题
                    self.highlight_title(widget)
                break
    
    def highlight_title(self, title_widget):
        """高亮显示标题"""
        # 保存原始样式
        if not hasattr(title_widget, 'original_style'):
            title_widget.original_style = title_widget.styleSheet()
        
        # 应用高亮样式
        highlight_style = title_widget.original_style + """
            background-color: #fff3cd;
            border: 2px solid #ffeaa7;
            border-radius: 5px;
            padding: 10px;
        """
        title_widget.setStyleSheet(highlight_style)
        
        # 3秒后恢复原始样式
        QTimer.singleShot(3000, lambda: self.restore_title_style(title_widget))
    
    def restore_title_style(self, title_widget):
        """恢复标题原始样式"""
        if hasattr(title_widget, 'original_style'):
            title_widget.setStyleSheet(title_widget.original_style)
    
    def on_search_text_changed(self, text):
        """搜索文本改变事件"""
        if not text.strip():
            # 清空搜索，显示所有内容
            for i in range(self.content_layout.count()):
                widget = self.content_layout.itemAt(i).widget()
                if widget:
                    widget.setVisible(True)
            return
        
        search_text = text.lower()
        
        # 遍历所有控件，隐藏不匹配的内容
        for i in range(self.content_layout.count()):
            widget = self.content_layout.itemAt(i).widget()
            if widget:
                if isinstance(widget, QLabel):
                    # 检查标签文本
                    widget_text = widget.text().lower()
                    widget.setVisible(search_text in widget_text)
                elif hasattr(widget, 'layout') and widget.layout():
                    # 检查代码块
                    if widget.layout().count() >= 2:
                        code_text_widget = widget.layout().itemAt(1).widget()
                        if isinstance(code_text_widget, QTextEdit):
                            code_text = code_text_widget.toPlainText().lower()
                            widget.setVisible(search_text in code_text)
                else:
                    # 其他控件默认显示
                    widget.setVisible(True)

class OutputTextEdit(QPlainTextEdit):
    """
    @class OutputTextEdit
    @brief 支持Ctrl+滚轮调整字体大小和右键菜单的输出文本编辑器
    
    修复了无法复制文本的问题，添加了完整的右键菜单功能
    """
    
    def __init__(self, parent=None):
        """
        @brief 构造函数
        @param parent 父组件
        
        初始化右键菜单和复制功能
        """
        super(OutputTextEdit, self).__init__(parent)
        self.setup_context_menu()
        self.setup_shortcuts()
    
    def setup_context_menu(self):
        """设置右键菜单"""
        # 启用自定义上下文菜单
        self.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.customContextMenuRequested.connect(self.show_context_menu)
    
    def setup_shortcuts(self):
        """设置快捷键"""
        # 添加Ctrl+A全选快捷键（Ctrl+C使用默认行为）
        select_all_shortcut = QAction("全选", self)
        select_all_shortcut.setShortcut(QKeySequence("Ctrl+A"))
        select_all_shortcut.triggered.connect(self.selectAll)
        self.addAction(select_all_shortcut)
    
    def show_context_menu(self, position):
        """显示右键菜单"""
        # 创建菜单
        menu = QMenu(self)
        
        # 复制动作
        copy_action = QAction("复制", self)
        copy_action.triggered.connect(self.copy_selected_text)
        menu.addAction(copy_action)
        
        # 复制全部动作
        copy_all_action = QAction("复制全部", self)
        copy_all_action.triggered.connect(self.copy_all_text)
        menu.addAction(copy_all_action)
        
        menu.addSeparator()
        
        # 清空动作
        clear_action = QAction("清空输出", self)
        clear_action.triggered.connect(self.clear)
        menu.addAction(clear_action)
        
        # 显示菜单
        menu.exec(self.mapToGlobal(position))
    
    def copy_selected_text(self):
        """复制选中的文本"""
        cursor = self.textCursor()
        if cursor.hasSelection():
            selected_text = cursor.selectedText()
            clipboard = QApplication.clipboard()
            clipboard.setText(selected_text)
            
            # 显示复制成功提示（使用无边框样式）
            msg_box = QMessageBox(self)
            msg_box.setText("复制成功！")
            msg_box.setStyleSheet("""
                QMessageBox {
                    background-color: #ffffff;
                    border: none;
                    padding: 10px;
                }
                QLabel {
                    color: #2c3e50;
                    font-size: 14px;
                }
                QPushButton {
                    background-color: #3498db;
                    color: white;
                    border: none;
                    padding: 5px 15px;
                    border-radius: 3px;
                    min-width: 80px;
                    outline: none;
                }
                QPushButton:hover {
                    background-color: #2980b9;
                }
            """)
            msg_box.exec()
            
            # 复制成功后自动取消选中，避免视觉干扰
            cursor.clearSelection()
            self.setTextCursor(cursor)
        else:
            # 如果没有选中文本，显示提示
            msg_box = QMessageBox(self)
            msg_box.setText("请先选择要复制的文本！")
            msg_box.setStyleSheet("""
                QMessageBox {
                    background-color: #ffffff;
                    border: none;
                    padding: 10px;
                }
                QLabel {
                    color: #2c3e50;
                    font-size: 14px;
                }
                QPushButton {
                    background-color: #3498db;
                    color: white;
                    border: none;
                    padding: 5px 15px;
                    border-radius: 3px;
                    min-width: 80px;
                    outline: none;
                }
                QPushButton:hover {
                    background-color: #2980b9;
                }
            """)
            msg_box.exec()
    
    def copy_all_text(self):
        """复制全部文本"""
        all_text = self.toPlainText()
        if all_text.strip():
            clipboard = QApplication.clipboard()
            clipboard.setText(all_text)
            # 显示复制成功提示（使用无边框样式）
            msg_box = QMessageBox(self)
            msg_box.setText("全部内容复制成功！")
            msg_box.setStyleSheet("""
                QMessageBox {
                    background-color: #ffffff;
                    border: none;
                    padding: 10px;
                }
                QLabel {
                    color: #2c3e50;
                    font-size: 14px;
                }
                QPushButton {
                    background-color: #3498db;
                    color: white;
                    border: none;
                    padding: 5px 15px;
                    border-radius: 3px;
                    min-width: 80px;
                    outline: none;
                }
                QPushButton:hover {
                    background-color: #2980b9;
                }
            """)
            msg_box.exec()
        else:
            # 如果内容为空，显示提示
            msg_box = QMessageBox(self)
            msg_box.setText("输出区域为空，没有内容可复制！")
            msg_box.setStyleSheet("""
                QMessageBox {
                    background-color: #ffffff;
                    border: none;
                    padding: 10px;
                }
                QLabel {
                    color: #2c3e50;
                    font-size: 14px;
                }
                QPushButton {
                    background-color: #3498db;
                    color: white;
                    border: none;
                    padding: 5px 15px;
                    border-radius: 3px;
                    min-width: 80px;
                    outline: none;
                }
                QPushButton:hover {
                    background-color: #2980b9;
                }
            """)
            msg_box.exec()
    
    def wheelEvent(self, event):
        """重写滚轮事件，支持Ctrl+滚轮调整字体大小"""
        if event.modifiers() & Qt.KeyboardModifier.ControlModifier:
            # 获取当前字体
            font = self.font()
            current_size = font.pointSize()
            
            # 根据滚轮方向调整字体大小
            delta = event.angleDelta().y()
            if delta > 0:
                # 向上滚动，增大字体
                new_size = min(current_size + 1, 72)
            else:
                # 向下滚动，减小字体
                new_size = max(current_size - 1, 6)
            
            # 应用新字体大小
            font.setPointSize(new_size)
            self.setFont(font)
            
            # 阻止默认滚动行为
            event.accept()
        else:
            # 正常滚动
            super().wheelEvent(event)

class CHPlusHighlighter(QSyntaxHighlighter):
    """
    @class CHPlusHighlighter
    @brief CH+语言语法高亮器
    
    性能优化：缓存正则表达式、优化关键字匹配、减少重复计算
    """
    def __init__(self, parent=None):
        """
        @brief 构造函数
        @param parent 父组件
        
        性能优化：预编译正则表达式、缓存关键字模式
        """
        super(CHPlusHighlighter, self).__init__(parent)
        
        # 默认关键字列表（性能优化：使用元组避免修改）
        self.default_keywords = (
            "定义", "如果", "否则", "否则如果", "当", "对于", "返回",
            "控制台输出", "控制台输入", "控制台换行", "导入", "系统命令行", "空类型",
            "整型", "字符串", "小数", "布尔型", "字符型", "结构体",
            "真", "假", "空", "退出循环", "下一层循环"
        )
        
        # 关键字列表（从配置文件加载，性能优化：使用列表副本）
        self.keywords = list(self.default_keywords)
        
        # 性能优化：预编译关键字正则表达式
        self.keyword_pattern = None
        self._compile_keyword_pattern()
        
        # 初始化颜色格式
        self.keyword_format = QTextCharFormat()
        self.comment_format = QTextCharFormat()
        self.string_format = QTextCharFormat()
        self.number_format = QTextCharFormat()
        self.operator_format = QTextCharFormat()
        self.identifier_format = QTextCharFormat()
        
        # 关键字和系统函数的详细解释
        self.keyword_documentation = {
            # 关键字
            "定义": "用于定义变量或常量的关键字。\n\n语法：定义(类型) 变量名 = 值;\n\n示例：定义(整型) x = 10;",
            "如果": '条件判断语句，用于根据条件执行不同的代码块。\n\n语法：如果 (条件) {\n    // 条件为真时执行的代码\n} 否则 {\n    // 条件为假时执行的代码\n}\n\n示例：如果 (x > 0) {\n    控制台输出("x是正数");\n} 否则 {\n    控制台输出("x是负数或零");\n}',
            "否则": "与如果语句配合使用，用于指定条件为假时执行的代码块。\n\n语法：如果 (条件) {\n    // 条件为真时执行的代码\n} 否则 {\n    // 条件为假时执行的代码\n}",
            "否则如果": "与如果语句配合使用，用于指定多个条件判断。\n\n语法：如果 (条件1) {\n    // 条件1为真时执行的代码\n} 否则如果 (条件2) {\n    // 条件2为真时执行的代码\n} 否则 {\n    // 所有条件都为假时执行的代码\n}",
            "当": "循环语句，用于当条件为真时重复执行代码块。\n\n语法：当 (条件) {\n    // 循环体代码\n}\n\n示例：当 (i < 10) {\n    控制台输出(i);\n    i = i + 1;\n}",
            "对于": "循环语句，用于指定初始化、条件和更新表达式的循环。\n\n语法：对于 (初始化; 条件; 更新) {\n    // 循环体代码\n}\n\n示例：对于 (i = 0; i < 10; i = i + 1) {\n    控制台输出(i);\n}",
            "返回": "用于从函数中返回值的关键字。\n\n语法：返回 值;\n\n示例：返回 x + y;",
            "控制台输出": '用于向控制台输出内容的关键字。\n\n语法：控制台输出(表达式);\n\n示例：控制台输出("Hello World!");',
            "控制台输入": "用于从控制台读取输入的关键字。\n\n语法：控制台输入(变量名);\n\n示例：控制台输入(name);",
            "控制台换行": "用于向控制台输出换行符的关键字。\n\n语法：控制台换行;",
            "导入": '用于导入其他CH+文件的关键字。\n\n语法：导入 "文件名.ch";\n\n示例：导入 "math.ch";',
            "系统命令行": '用于执行系统命令的关键字。\n\n语法：系统命令行 "命令";\n\n示例：系统命令行 "dir";',
            "空类型": "表示函数没有返回值的类型。\n\n语法：空类型 函数名() {\n    // 函数体\n}",
            "整型": "整数类型，用于存储整数值。\n\n示例：定义(整型) x = 10;",
            "字符串": "字符串类型，用于存储文本。\n\n示例：定义(字符串) s = \"Hello\";\n示例：定义(字符串) s = \"你好\";\n示例：定义(字符串) s = \"Hello World\";",
            "小数": "浮点数类型，用于存储带小数点的数值。\n\n示例：定义(小数) x = 3.14;",
            "布尔型": "布尔类型，用于存储真或假的值。\n\n示例：定义(布尔型) flag = 真;",
            "字符型": "字符类型，用于存储单个字符。\n\n示例：定义(字符型) c = 'A';",
            "结构体": "用于定义自定义数据类型的关键字。\n\n语法：结构体 结构体名 {\n    成员类型 成员名;\n    // 其他成员\n}\n\n示例：结构体 点 {\n    整型 x;\n    整型 y;\n    字符串 颜色;\n}\n\n使用结构体：\n定义(点) 点一;\n点一.x = 100;\n点一.y = 200;\n点一.颜色 = \"红色\";\n控制台输出(\"Point: x=\" + 点一.x + \", y=\" + 点一.y);",
            "数组": "数组是存储多个相同类型元素的集合。\n\n语法：定义(类型) 数组名[大小];\n\n示例：定义(整型) numbers[5];\n示例：numbers[0] = 10;\n示例：numbers[1] = 20;\n示例：控制台输出(numbers[0]); // 输出 10\n\n数组遍历示例：\n对于 (定义(整型) i = 0; i < 5; i = i + 1) {\n    控制台输出(numbers[i]);\n}",
            "真": "布尔类型的真值。\n\n示例：定义(布尔型) flag = 真;",
            "假": "布尔类型的假值。\n\n示例：定义(布尔型) flag = 假;",
            "退出循环": "用于退出循环的关键字。\n\n语法：退出循环;\n\n示例：当 (i < 10) {\n    如果 (i == 5) {\n        退出循环;\n    }\n    控制台输出(i);\n    i = i + 1;\n}",
            "下一层循环": "用于跳过当前循环的剩余部分，进入下一次循环的关键字。\n\n语法：下一层循环;\n\n示例：对于 (i = 0; i < 10; i = i + 1) {\n    如果 (i % 2 == 0) {\n        下一层循环;\n    }\n    控制台输出(i);\n}",
            
            # 系统函数
            "控制台输出()": '向控制台输出指定的内容。\n\n参数：一个或多个表达式，用于指定要输出的内容。\n\n示例：控制台输出("Hello");\n示例：控制台输出("x = ", x);',
            "控制台输入()": "从控制台读取用户输入并存储到指定变量中。\n\n参数：变量名，用于存储读取的输入。\n\n示例：控制台输入(name);",
            "控制台换行()": "向控制台输出一个换行符。\n\n参数：无。\n\n示例：控制台换行;",
            "系统命令行()": '执行指定的系统命令。\n\n参数：字符串表达式，指定要执行的命令。\n\n示例：系统命令行 "dir";\n示例：系统命令行 "ls -l";\n示例：系统命令行 "echo Hello";',
            "文件写入": '将内容写入到指定文件。\n\n参数：文件名（字符串）和内容（字符串）。\n\n示例：文件写入("test.txt", "Hello World");',
            "文件读取": '从指定文件读取内容并存储到变量中。\n\n参数：文件名（字符串）和变量名。\n\n示例：文件读取("test.txt", 文件内容);',
            "文件追加": '向指定文件追加内容。\n\n参数：文件名（字符串）和内容（字符串）。\n\n示例：文件追加("test.txt", "\\n这是追加的内容");',
            "长度()": '返回字符串的长度。\n\n参数：字符串表达式。\n\n返回值：整型，字符串的长度。\n\n示例：长度("Hello") // 返回 5',
            "字符转整型()": "将字符转换为对应的ASCII码值。\n\n参数：字符表达式。\n\n返回值：整型，字符的ASCII码值。\n\n示例：字符转整型('A') // 返回 65",
            "转小写()": '将字符串转换为小写。\n\n参数：字符串表达式。\n\n返回值：字符串，转换后的小写字符串。\n\n示例：导入("ch_Lib/string.ch");\n定义(字符串) result = 转小写("HELLO"); // 返回 "hello"',
            "转大写()": '将字符串转换为大写。\n\n参数：字符串表达式。\n\n返回值：字符串，转换后的大写字符串。\n\n示例：导入("ch_Lib/string.ch");\n定义(字符串) result = 转大写("hello"); // 返回 "HELLO"',
            "去空白()": '去除字符串首尾的空白字符。\n\n参数：字符串表达式。\n\n返回值：字符串，去除空白后的字符串。\n\n示例：导入("ch_Lib/string.ch");\n定义(字符串) result = 去空白("  Hello  "); // 返回 "Hello"',
            "查找()": '在字符串中查找子串的位置。\n\n参数：字符串和子串。\n\n返回值：整型，子串的位置，未找到返回-1。\n\n示例：导入("ch_Lib/string.ch");\n定义(整型) pos = 查找("Hello World", "World"); // 返回 6',
            "替换()": '替换字符串中的子串。\n\n参数：原字符串、旧子串、新子串。\n\n返回值：字符串，替换后的字符串。\n\n示例：导入("ch_Lib/string.ch");\n定义(字符串) result = 替换("Hello World", "World", "CH+"); // 返回 "Hello CH+"',
            "全部替换()": '替换字符串中所有匹配的子串。\n\n参数：原字符串、旧子串、新子串。\n\n返回值：字符串，全部替换后的字符串。\n\n示例：导入("ch_Lib/string.ch");\n定义(字符串) result = 全部替换("Hello Hello", "Hello", "Hi"); // 返回 "Hi Hi"',
            "子串()": '提取字符串的子串。\n\n参数：字符串、起始位置、长度。\n\n返回值：字符串，提取的子串。\n\n示例：导入("ch_Lib/string.ch");\n定义(字符串) result = 子串("Hello World", 0, 5); // 返回 "Hello"',
            "连接()": '连接两个字符串。\n\n参数：两个字符串。\n\n返回值：字符串，连接后的字符串。\n\n示例：导入("ch_Lib/string.ch");\n定义(字符串) result = 连接("Hello", " World"); // 返回 "Hello World"',
            "整数转字符串()": '将整数转换为字符串。\n\n参数：整数表达式。\n\n返回值：字符串，转换后的字符串。\n\n示例：整数转字符串(123) // 返回 "123"',
            
            # 数学库函数（需要导入 "ch_Lib/math.ch"）
            "sin()": '计算角度的正弦值。\n\n参数：小数，角度值（度）。\n\n返回值：小数，正弦值。\n\n示例：导入("ch_Lib/math.ch");\n定义(小数) result = sin(45.0); // 返回约 0.707',
            "cos()": '计算角度的余弦值。\n\n参数：小数，角度值（度）。\n\n返回值：小数，余弦值。\n\n示例：导入("ch_Lib/math.ch");\n定义(小数) result = cos(45.0); // 返回约 0.707',
            "tan()": '计算角度的正切值。\n\n参数：小数，角度值（度）。\n\n返回值：小数，正切值。\n\n示例：导入("ch_Lib/math.ch");\n定义(小数) result = tan(45.0); // 返回约 1.0',
            "sqrt()": '计算平方根。\n\n参数：小数，非负数。\n\n返回值：小数，平方根。\n\n示例：导入("ch_Lib/math.ch");\n定义(小数) result = sqrt(16.0); // 返回 4.0',
            "pow()": '计算幂运算。\n\n参数：底数（小数）和指数（小数）。\n\n返回值：小数，底数的指数次幂。\n\n示例：导入("ch_Lib/math.ch");\n定义(小数) result = pow(2.0, 3.0); // 返回 8.0',
            "abs()": '计算绝对值。\n\n参数：整型或小数。\n\n返回值：整型或小数，绝对值。\n\n示例：导入("ch_Lib/math.ch");\n定义(整型) result = abs(-10); // 返回 10',
            "PI": '圆周率常数，约等于 3.141592653589793。\n\n示例：导入("ch_Lib/math.ch");\n定义(小数) area = PI * 半径 * 半径;',
            "E": '自然常数，约等于 2.718281828459045。\n\n示例：导入("ch_Lib/math.ch");\n定义(小数) value = E;',
            
            "主函数()": '程序的入口点，当程序运行时会自动执行主函数中的代码。\n\n语法：空类型 主函数() {\n    // 函数体\n}\n\n示例：空类型 主函数() {\n    控制台输出("Hello World!");\n}'
        }
        
        # 设置默认颜色
        self.set_default_colors()
        
        # 正则表达式模式（性能优化：预编译）
        self.comment_pattern = QRegularExpression(r'//.*')
        self.string_pattern = QRegularExpression(r'"(?:\\.|[^"\\])*"')
        self.number_pattern = QRegularExpression(r'\b\d+(\.\d+)?\b')
        self.operator_pattern = QRegularExpression(r'[=+\-*/<>!&|]+')
        # 简化标识符模式，只匹配英文和数字，避免Unicode问题
        self.identifier_pattern = QRegularExpression(r'\b[a-zA-Z_][a-zA-Z0-9_]*\b')
        
        # 性能优化：预编译正则表达式
        self._precompile_patterns()
    
    def _compile_keyword_pattern(self):
        """
        @brief 预编译关键字正则表达式
        
        性能优化：避免每次高亮时重新构建正则表达式
        """
        if self.keywords:
            # 构建关键字模式，使用单词边界确保精确匹配
            pattern = r'\b(' + '|'.join(re.escape(keyword) for keyword in self.keywords) + r')\b'
            self.keyword_pattern = QRegularExpression(pattern)
    
    def _precompile_patterns(self):
        """
        @brief 预编译所有正则表达式
        
        性能优化：提前编译正则表达式，提高匹配速度
        """
        # 预编译正则表达式选项
        options = QRegularExpression.PatternOption.DotMatchesEverythingOption
        
        self.comment_pattern.optimize()
        self.string_pattern.optimize()
        self.number_pattern.optimize()
        self.operator_pattern.optimize()
        self.identifier_pattern.optimize()
        
        if self.keyword_pattern:
            self.keyword_pattern.optimize()
    
    def set_default_colors(self):
        """
        @brief 设置默认颜色
        
        性能优化：缓存颜色对象，避免重复创建
        """
        # 性能优化：缓存颜色对象
        if not hasattr(self, '_cached_colors'):
            self._cached_colors = {
                'keyword': QColor("#0000FF"),
                'comment': QColor("#808080"),
                'string': QColor("#FF0000"),
                'number': QColor("#FF8C00"),
                'operator': QColor("#000000"),
                'identifier': QColor("#000000")
            }
        
        # 关键字颜色
        self.keyword_format.setForeground(self._cached_colors['keyword'])
        self.keyword_format.setFontWeight(QFont.Weight.Bold)
        
        # 注释颜色（灰色）
        self.comment_format.setForeground(self._cached_colors['comment'])
        self.comment_format.setFontItalic(True)
        
        # 字符串颜色
        self.string_format.setForeground(self._cached_colors['string'])
        
        # 数字颜色
        self.number_format.setForeground(self._cached_colors['number'])
        
        # 运算符颜色
        self.operator_format.setForeground(self._cached_colors['operator'])
        
        # 标识符颜色
        self.identifier_format.setForeground(self._cached_colors['identifier'])
    
    def update_colors(self, settings):
        """根据设置更新颜色"""
        # 更新关键字颜色
        self.keyword_format.setForeground(QColor(settings["color_keyword"]))
        self.keyword_format.setFontWeight(QFont.Weight.Bold)
        
        # 更新注释颜色
        self.comment_format.setForeground(QColor(settings["color_comment"]))
        self.comment_format.setFontItalic(True)
        
        # 更新字符串颜色
        self.string_format.setForeground(QColor(settings["color_string"]))
        
        # 更新数字颜色
        self.number_format.setForeground(QColor(settings["color_number"]))
        
        # 更新运算符颜色
        self.operator_format.setForeground(QColor(settings["color_operator"]))
        
        # 更新标识符颜色
        self.identifier_format.setForeground(QColor(settings["color_identifier"]))
        
        self.rehighlight()

    def update_keywords(self, keywords):
        """更新关键字列表"""
        self.keywords = keywords if keywords else self.default_keywords.copy()
        self.rehighlight()
    
    def update_color(self, color):
        """更新关键字颜色"""
        self.keyword_format.setForeground(color)
        self.rehighlight()

    def highlightBlock(self, text):
        """高亮文本块"""
        # 先高亮字符串
        match_iterator = self.string_pattern.globalMatch(text)
        while match_iterator.hasNext():
            match = match_iterator.next()
            self.setFormat(match.capturedStart(), match.capturedLength(), self.string_format)
        
        # 高亮数字（跳过字符串和注释）
        match_iterator = self.number_pattern.globalMatch(text)
        while match_iterator.hasNext():
            match = match_iterator.next()
            start = match.capturedStart()
            # 检查是否在注释中
            comment_pos = text.find('//')
            if comment_pos == -1 or start < comment_pos:
                self.setFormat(start, match.capturedLength(), self.number_format)
        
        # 高亮运算符（跳过字符串和注释）
        match_iterator = self.operator_pattern.globalMatch(text)
        while match_iterator.hasNext():
            match = match_iterator.next()
            start = match.capturedStart()
            # 检查是否在注释中
            comment_pos = text.find('//')
            if comment_pos == -1 or start < comment_pos:
                self.setFormat(start, match.capturedLength(), self.operator_format)
        
        # 高亮关键字（跳过字符串和注释）
        for keyword in self.keywords:
            # 使用简单的字符串匹配来避免正则表达式问题
            start_pos = 0
            while True:
                pos = text.find(keyword, start_pos)
                if pos == -1:
                    break
                # 检查是否是完整的单词
                if (pos == 0 or not text[pos-1].isalnum()) and \
                   (pos + len(keyword) == len(text) or not text[pos + len(keyword)].isalnum()):
                    # 检查是否在注释中
                    comment_pos = text.find('//')
                    if comment_pos == -1 or pos < comment_pos:
                        self.setFormat(pos, len(keyword), self.keyword_format)
                start_pos = pos + len(keyword)
        
        # 高亮标识符（非关键字）- 简化处理，避免复杂正则表达式
        # 使用简单的英文标识符匹配
        match_iterator = self.identifier_pattern.globalMatch(text)
        while match_iterator.hasNext():
            match = match_iterator.next()
            # 检查是否是关键字
            is_keyword = False
            for keyword in self.keywords:
                if match.captured() == keyword:
                    is_keyword = True
                    break
            if not is_keyword:
                start = match.capturedStart()
                # 检查是否在注释中
                comment_pos = text.find('//')
                if comment_pos == -1 or start < comment_pos:
                    self.setFormat(start, match.capturedLength(), self.identifier_format)
        
        # 最后高亮注释（覆盖其他所有高亮，包括数字）
        match_iterator = self.comment_pattern.globalMatch(text)
        while match_iterator.hasNext():
            match = match_iterator.next()
            self.setFormat(match.capturedStart(), match.capturedLength(), self.comment_format)

class WorkerSignals(QObject):
    """工作线程信号"""
    output_signal = pyqtSignal(str)
    input_signal = pyqtSignal(str)

class CHPlusIDE(QMainWindow):
    def __init__(self):
        super(CHPlusIDE, self).__init__()
        
        # 设置窗口标题和大小
        self.setWindowTitle("CH+ 中文编辑器")
        
        # 设置窗口图标
        if os.path.exists("app.ico"):
            self.setWindowIcon(QIcon("app.ico"))
        
        # 默认全屏运行
        screen = QApplication.primaryScreen()
        screen_geometry = screen.availableGeometry()
        self.setGeometry(screen_geometry)
        self.showMaximized()
        
        # 配置文件路径
        self.config_file = "chplus_ide.ini"
        
        # 当前文件
        self.current_file = ""
        
        # 撤销/重做栈
        self.undo_stack = []
        self.redo_stack = []
        self.max_undo = 100
        
        # 运行选项
        self.auto_format = False
        self.no_format = False
        self.debug_level = "0"
        self.memory_option = "内存"
        self.clear_output = False
        self.user_input_value = ""
        
        # 字符转换选项
        self.convert_chinese_to_english = False
        self.exclude_strings_from_conversion = True
        self.current_process = None
        self.input_event = threading.Event()
        
        # 设置选项
        self.settings = {
            # 环境-外观设置
            "theme": "浅色",
            "font": "Microsoft YaHei UI",
            "font_size": 11,
            "icon_set": "新外观",
            "use_custom_icons": False,
            "icon_scale": 1.0,
            "language": "简体中文",
            "wheel_scroll": True,
            
            # 编辑器-通用设置
            "indent_auto": True,
            "indent_use_spaces": False,
            "tab_width": 4,
            "indent_guide": True,
            "indent_fill": False,
            "cursor_home_non_space": True,
            "cursor_end_non_space": True,
            "cursor_remember_column": True,
            "insert_cursor": "竖线",
            "overwrite_cursor": "块",
            "cursor_use_text_color": True,
            "highlight_bracket": True,
            "highlight_word": True,
            "scroll_auto_hide": False,
            "scroll_end_to_left": False,
            "scroll_last_to_top": True,
            "scroll_half_page": False,
            "scroll_speed": 3,
            "scroll_drag_speed": 10,
            "scroll_edge": False,
            "scroll_edge_width": 80,
            "scroll_edge_color": "#fffff0",
            
            # 中文转换设置
            "chinese_convert_enabled": False,
            "chinese_exclude_strings": True,
            
            # 编辑器-格式设置
            "format_space_between_sections": False,
            "format_space_between_all_sections": False,
            "format_remove_extra_lines": False,
            "format_remove_extra_lines_count": 1,
            "format_space_after_comma": True,
            "format_space_around_operator": True,
            "format_space_inside_paren": False,
            "format_space_both_paren": False,
            "format_space_outside_paren": False,
            "format_space_multi_paren": False,
            "format_space_after_stmt": True,
            "format_remove_extra_space": False,
            
            # 语言-自定义关键字设置
            "custom_keywords_enabled": False,
            "custom_keywords": [],
            
            # 颜色配置设置
            "color_keyword": "#0000FF",
            "color_comment": "#808080",
            "color_string": "#FF0000",
            "color_number": "#FF8C00",
            "color_operator": "#000000",
            "color_identifier": "#000000",
            "color_background": "#FFFFFF",
            "color_line_number": "#808080",
            "color_current_line": "#F0F8FF",
            "color_selection": "#ADD8E6",
            "color_cursor": "#000000"
        }
        
        # 初始化UI组件
        self.init_ui()
        
        # 加载配置文件
        self.load_config()
        
        # 应用配置设置
        self.apply_settings()
        
        # 初始化库函数树
        self.initialize_library_tree()
        
        # 恢复上一次打开的文件
        self.restore_last_file()
    
    def init_ui(self):
        """初始化UI组件"""
        # 创建菜单栏
        self.create_menu_bar()
        
        # 创建中央部件
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # 创建主布局，设置边距为0以去掉上方空白
        main_layout = QVBoxLayout(central_widget)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.setSpacing(2)
        
        # 创建分割器
        splitter = QSplitter(Qt.Orientation.Vertical)
        splitter.setHandleWidth(2)
        main_layout.addWidget(splitter)
        
        # 创建顶部区域（包含左侧树和编辑器）
        top_widget = QWidget()
        top_layout = QHBoxLayout(top_widget)
        top_layout.setContentsMargins(2, 2, 2, 2)
        top_layout.setSpacing(2)
        
        # 创建左侧库函数树
        self.tree_widget = QTreeWidget()
        self.tree_widget.setHeaderHidden(True)
        self.tree_widget.setFixedWidth(250)
        self.tree_widget.setStyleSheet("""
            QTreeWidget {
                background-color: #FFFFFF;
                color: #000000;
                border: 1px solid #CCCCCC;
            }
            QTreeWidget::item {
                padding: 2px;
            }
            QTreeWidget::item:selected {
                background-color: #0078D4;
                color: #FFFFFF;
            }
        """)
        top_layout.addWidget(self.tree_widget)
        
        # 创建代码编辑器容器
        editor_container = QWidget()
        editor_container_layout = QHBoxLayout(editor_container)
        editor_container_layout.setContentsMargins(0, 0, 0, 0)
        editor_container_layout.setSpacing(0)
        editor_container.setStyleSheet("""
            QWidget {
                background-color: #FFFFFF;
                border: 1px solid #CCCCCC;
            }
        """)
        
        # 创建代码编辑器
        self.editor = CHPlusTextEdit(self)
        self.editor.setFont(QFont("Consolas", 12))
        self.editor.setStyleSheet("""
            QTextEdit {
                background-color: #FFFFFF;
                color: #000000;
                selection-background-color: #0078D4;
                selection-color: #FFFFFF;
                border: none;
            }
        """)
        self.editor.textChanged.connect(self.on_text_change)
        editor_container_layout.addWidget(self.editor)
        
        # 设置初始光标位置
        cursor = self.editor.textCursor()
        cursor.movePosition(QTextCursor.MoveOperation.Start)
        self.editor.setTextCursor(cursor)
        self.editor.setFocus()
        
        # 确保编辑器有初始内容，避免光标位置问题
        if not self.editor.toPlainText():
            self.editor.setPlainText("")
        
        top_layout.addWidget(editor_container)
        
        # 添加语法高亮
        self.highlighter = CHPlusHighlighter(self.editor.document())
        
        # 将顶部区域添加到分割器
        splitter.addWidget(top_widget)
        
        # 创建输出区域
        self.output = OutputTextEdit()
        self.output.setFont(QFont("Consolas", 10))
        self.output.setStyleSheet("""
            QTextEdit {
                background-color: #FFFFFF;
                color: #000000;
                border: 1px solid #CCCCCC;
                outline: none;
            }
            QTextEdit:focus {
                background-color: #FFFFFF;
                color: #000000;
                border: 1px solid #CCCCCC;
                outline: none;
            }
        """)
        self.output.setFixedHeight(113)
        self.output.setReadOnly(True)
        splitter.addWidget(self.output)
        
        # 绑定快捷键
        self.bind_shortcuts()
    
    def create_menu_bar(self):
        """创建菜单栏"""
        menubar = self.menuBar()
        
        # 文件菜单
        file_menu = menubar.addMenu("文件(F)")
        
        new_action = QAction("新建(N)", self)
        new_action.setShortcut(QKeySequence("Ctrl+N"))
        new_action.triggered.connect(self.new_file)
        file_menu.addAction(new_action)
        
        open_action = QAction("打开(O)", self)
        open_action.setShortcut(QKeySequence("Ctrl+O"))
        open_action.triggered.connect(self.open_file)
        file_menu.addAction(open_action)
        
        close_action = QAction("关闭(C)", self)
        close_action.setShortcut(QKeySequence("Ctrl+W"))
        close_action.triggered.connect(self.close_file)
        file_menu.addAction(close_action)
        
        save_action = QAction("保存(S)", self)
        save_action.setShortcut(QKeySequence("Ctrl+S"))
        save_action.triggered.connect(self.save_file)
        file_menu.addAction(save_action)
        
        save_as_action = QAction("另存为(A)", self)
        save_as_action.setShortcut(QKeySequence("Ctrl+Shift+S"))
        save_as_action.triggered.connect(self.save_as_file)
        file_menu.addAction(save_as_action)
        
        file_menu.addSeparator()
        
        exit_action = QAction("退出(X)", self)
        exit_action.triggered.connect(self.close)
        file_menu.addAction(exit_action)
        
        # 编辑菜单
        edit_menu = menubar.addMenu("编辑(E)")
        
        undo_action = QAction("撤销(U)", self)
        undo_action.setShortcut(QKeySequence("Ctrl+Z"))
        undo_action.triggered.connect(self.undo)
        edit_menu.addAction(undo_action)
        
        redo_action = QAction("重复(R)", self)
        redo_action.setShortcut(QKeySequence("Ctrl+Y"))
        redo_action.triggered.connect(self.redo)
        edit_menu.addAction(redo_action)
        
        edit_menu.addSeparator()
        
        copy_action = QAction("复制(C)", self)
        copy_action.setShortcut(QKeySequence("Ctrl+C"))
        copy_action.triggered.connect(self.copy_text)
        edit_menu.addAction(copy_action)
        
        cut_action = QAction("剪切(T)", self)
        cut_action.setShortcut(QKeySequence("Ctrl+X"))
        cut_action.triggered.connect(self.cut_text)
        edit_menu.addAction(cut_action)
        
        paste_action = QAction("粘贴(P)", self)
        paste_action.setShortcut(QKeySequence("Ctrl+V"))
        paste_action.triggered.connect(self.paste_text)
        edit_menu.addAction(paste_action)
        
        delete_line_action = QAction("删除行(D)", self)
        delete_line_action.setShortcut(QKeySequence("F10"))
        delete_line_action.triggered.connect(self.delete_line)
        edit_menu.addAction(delete_line_action)
        
        edit_menu.addSeparator()
        
        find_action = QAction("开始寻找(F)", self)
        find_action.setShortcut(QKeySequence("Ctrl+F"))
        find_action.triggered.connect(self.find)
        edit_menu.addAction(find_action)
        
        find_mark_action = QAction("开始标记寻找单词(F)", self)
        find_mark_action.setShortcut(QKeySequence("Ctrl+Q"))
        find_mark_action.triggered.connect(self.find_mark)
        edit_menu.addAction(find_mark_action)
        
        highlight_action = QAction("高亮标记待寻找单词(G)", self)
        highlight_action.setShortcut(QKeySequence("Ctrl+Q"))
        highlight_action.triggered.connect(self.find_mark)
        edit_menu.addAction(highlight_action)
        
        find_next_action = QAction("寻找下一个(N)", self)
        find_next_action.setShortcut(QKeySequence("F3"))
        find_next_action.triggered.connect(self.find_next)
        edit_menu.addAction(find_next_action)
        
        find_prev_action = QAction("寻找上一个(V)", self)
        find_prev_action.setShortcut(QKeySequence("Shift+F3"))
        find_prev_action.triggered.connect(self.find_prev)
        edit_menu.addAction(find_prev_action)
        
        replace_action = QAction("寻找替换(H)", self)
        replace_action.setShortcut(QKeySequence("Ctrl+H"))
        replace_action.triggered.connect(self.find_replace)
        edit_menu.addAction(replace_action)
        
        find_all_action = QAction("整体搜寻(E)", self)
        find_all_action.setShortcut(QKeySequence("Ctrl+Alt+F"))
        find_all_action.triggered.connect(self.find_all)
        edit_menu.addAction(find_all_action)
        
        # 运行菜单
        run_menu = menubar.addMenu("运行(R)")
        
        run_action = QAction("运行(R)", self)
        run_action.setShortcut(QKeySequence("F5"))
        run_action.triggered.connect(self.run_current_file)
        run_menu.addAction(run_action)
        
        run_menu.addSeparator()
        
        options_action = QAction("选项(O)", self)
        options_action.triggered.connect(self.show_options)
        run_menu.addAction(options_action)
        
        # 帮助菜单
        help_menu = menubar.addMenu("帮助(H)")
        
        settings_action = QAction("设置(S)", self)
        settings_action.triggered.connect(self.show_settings)
        help_menu.addAction(settings_action)
        
        help_menu.addSeparator()
        
        about_action = QAction("关于(A)", self)
        about_action.triggered.connect(self.show_about)
        help_menu.addAction(about_action)
    
    def bind_shortcuts(self):
        """绑定快捷键"""
        # 已经通过QAction绑定了大部分快捷键
        # 这里可以添加额外的快捷键
        pass
    
    def on_text_change(self):
        """文本变化时的处理"""
        self.save_state()
    
    def save_state(self):
        """保存当前状态到撤销栈"""
        current_content = self.editor.toPlainText()
        if not self.undo_stack or self.undo_stack[-1] != current_content:
            self.undo_stack.append(current_content)
            if len(self.undo_stack) > self.max_undo:
                self.undo_stack.pop(0)
            self.redo_stack.clear()
    

    def initialize_library_tree(self):
        """初始化库函数树状结构"""
        # 清空树
        self.tree_widget.clear()
        
        # 关键字（作为顶级节点）
        keywords = QTreeWidgetItem(self.tree_widget, ["关键字"])
        for keyword in self.highlighter.keywords:
            item = QTreeWidgetItem([keyword])
            keywords.addChild(item)
        
        # 系统函数（作为顶级节点，按功能分类）
        system = QTreeWidgetItem(self.tree_widget, ["系统函数"])
        
        # 控制台相关函数
        console = QTreeWidgetItem(system, ["控制台"])
        console.addChild(QTreeWidgetItem(["控制台输出()"]))
        console.addChild(QTreeWidgetItem(["控制台输入()"]))
        console.addChild(QTreeWidgetItem(["控制台换行()"]))
        
        # 命令行相关函数
        command = QTreeWidgetItem(system, ["命令行"])
        command.addChild(QTreeWidgetItem(["系统命令行()"]))
        
        # 文件读写相关函数
        file_io = QTreeWidgetItem(system, ["文件读写"])
        file_io.addChild(QTreeWidgetItem(["文件写入"]))
        file_io.addChild(QTreeWidgetItem(["文件读取"]))
        file_io.addChild(QTreeWidgetItem(["文件追加"]))
        
        # 字符串相关函数
        string = QTreeWidgetItem(system, ["字符串"])
        string.addChild(QTreeWidgetItem(["长度()"]))
        string.addChild(QTreeWidgetItem(["字符转整型()"]))
        string.addChild(QTreeWidgetItem(["转小写()"]))
        string.addChild(QTreeWidgetItem(["整数转字符串()"]))
        
        # 主函数
        main_func = QTreeWidgetItem(system, ["主函数"])
        main_func.addChild(QTreeWidgetItem(["主函数()"]))
        
        # 连接树状结构的双击事件
        self.tree_widget.itemDoubleClicked.connect(self.on_tree_item_double_clicked)
        
        # 所有节点默认不展开
        # 移除了setExpanded(True)调用，使所有栏默认都不展开
    
    def on_tree_item_double_clicked(self, item, column):
        """处理树状结构项的双击事件"""
        text = item.text(0)
        
        # 移除括号，如果有的话
        keyword = text.replace("()", "")
        
        # 检查是否是关键字或系统函数
        if keyword in self.highlighter.keyword_documentation:
            # 显示关键字文档
            self.show_keyword_documentation(keyword)
    
    def copy_text(self):
        """复制选中的文本"""
        self.editor.copy()
    
    def paste_text(self):
        """粘贴剪贴板内容"""
        self.save_state()
        self.editor.paste()
    
    def cut_text(self):
        """剪切选中的文本"""
        self.save_state()
        self.editor.cut()
    
    def undo(self):
        """撤销操作"""
        if len(self.undo_stack) > 1:
            current = self.undo_stack.pop()
            self.redo_stack.append(current)
            previous = self.undo_stack[-1]
            self.editor.setPlainText(previous)
    
    def redo(self):
        """重做操作"""
        if self.redo_stack:
            next_state = self.redo_stack.pop()
            self.undo_stack.append(next_state)
            self.editor.setPlainText(next_state)
    
    def delete_line(self):
        """删除当前行"""
        cursor = self.editor.textCursor()
        cursor.select(QTextCursor.SelectionType.LineUnderCursor)
        self.save_state()
        cursor.removeSelectedText()
    
    def new_file(self):
        """新建文件"""
        self.current_file = ""
        self.editor.clear()
        self.undo_stack.clear()
        self.redo_stack.clear()
        self.save_state()
        self.save_last_file()
        self.show_output("新建文件")
    
    def open_file(self):
        """打开文件"""
        file_path, _ = QFileDialog.getOpenFileName(
            self, "打开文件", "", "CH+文件 (*.ch);;所有文件 (*.*)"
        )
        if file_path:
            self.current_file = file_path
            try:
                with open(file_path, "r", encoding="utf-8") as f:
                    content = f.read()
                self.editor.setPlainText(content)
                # 设置光标到文档开始
                cursor = self.editor.textCursor()
                cursor.movePosition(QTextCursor.MoveOperation.Start)
                self.editor.setTextCursor(cursor)
                self.undo_stack.clear()
                self.redo_stack.clear()
                self.save_state()
                self.save_last_file()
                self.show_output(f"已打开文件: {file_path}")
            except Exception as e:
                self.show_output(f"错误：{str(e)}")
    
    def save_file(self):
        """保存文件"""
        if not self.current_file:
            self.save_as_file()
        else:
            try:
                content = self.editor.toPlainText()
                with open(self.current_file, "w", encoding="utf-8") as f:
                    f.write(content)
                self.show_output(f"已保存文件: {self.current_file}")
            except Exception as e:
                self.show_output(f"错误：{str(e)}")
    
    def save_as_file(self):
        """另存为文件"""
        file_path, _ = QFileDialog.getSaveFileName(
            self, "另存为", "", "CH+文件 (*.ch);;所有文件 (*.*)"
        )
        if file_path:
            try:
                content = self.editor.toPlainText()
                with open(file_path, "w", encoding="utf-8") as f:
                    f.write(content)
                self.current_file = file_path
                self.save_last_file()
                self.show_output(f"已另存为: {self.current_file}")
            except Exception as e:
                self.show_output(f"错误：{str(e)}")
    
    def close_file(self):
        """关闭文件"""
        if self.current_file:
            self.show_output(f"已关闭文件: {self.current_file}")
        self.current_file = ""
        self.editor.clear()
        self.undo_stack.clear()
        self.redo_stack.clear()
        self.save_state()
    
    def run_current_file(self):
        """运行当前文件"""
        if not self.current_file:
            self.show_output("错误：没有打开的文件")
            return
        
        try:
            # 先保存文件
            self.save_file()
            
            # 每次运行清屏
            if self.clear_output:
                self.output.clear()
            
            # 获取当前脚本所在目录
            script_dir = os.path.dirname(os.path.abspath(__file__))
            
            # 构建运行命令
            run_cmd = [os.path.join(script_dir, "chplus.exe")]
            
            # 添加命令行选项
            if self.auto_format:
                run_cmd.append("-a")
            if self.no_format:
                run_cmd.append("-n")
            
            # 添加调试模式
            if self.debug_level != "0":
                run_cmd.extend(["-d", self.debug_level])
            
            # 添加内存存储选项
            if self.memory_option == "无保留":
                run_cmd.extend(["-t", "reserve"])
            elif self.memory_option == "文本":
                run_cmd.append("-t")
            elif self.memory_option == "内存":
                run_cmd.extend(["-t", "memory"])
            
            # 添加文件路径
            run_cmd.append(self.current_file)
            
            # 构建cmd命令
            cmd_str = " ".join(run_cmd)
            
            # 创建信号对象
            signals = WorkerSignals()
            signals.output_signal.connect(self.show_output)
            signals.input_signal.connect(self.get_user_input)
            
            # 在新线程中执行命令
            def execute_run():
                try:
                    # 使用os.system执行命令，确保在新cmd窗口中运行
                    if sys.platform == "win32":
                        # Windows平台：使用start命令在新窗口中运行，并添加pause（确保在换行后执行）
                        full_cmd = f'start "CH+运行" /wait cmd /c "cd /d {script_dir} && {cmd_str} && echo. && pause"'
                    else:
                        # Linux/macOS平台：使用终端运行
                        full_cmd = f'cd {script_dir} && {cmd_str}'
                    os.system(full_cmd)
                    
                except Exception as e:
                    signals.output_signal.emit(f"运行错误：{str(e)}")
            
            # 启动新线程
            threading.Thread(target=execute_run).start()
            
        except Exception as e:
            self.show_output(f"运行错误：{str(e)}")
    
    def get_user_input(self, prompt):
        """获取用户输入"""
        self.user_input_value = ""
        input_text, ok = QInputDialog.getText(self, "输入", prompt)
        if ok and input_text is not None:
            self.user_input_value = input_text + "\n"
        # 设置事件，表示输入已完成
        self.input_event.set()
        return self.user_input_value
    
    def find(self):
        """开始寻找"""
        text, ok = QInputDialog.getText(self, "寻找", "查找:")
        if ok and text:
            cursor = self.editor.textCursor()
            document = self.editor.document()
            
            # 从当前位置开始查找
            pos = cursor.position()
            match = document.find(text, pos)
            
            if not match.isNull():
                self.editor.setTextCursor(match)
                self.editor.ensureCursorVisible()
            else:
                # 从文档开始查找
                match = document.find(text, 0)
                if not match.isNull():
                    self.editor.setTextCursor(match)
                    self.editor.ensureCursorVisible()
                else:
                    self.show_output(f"未找到: {text}")
    
    def find_mark(self):
        """开始标记寻找单词"""
        pass
    
    def find_next(self):
        """寻找下一个"""
        pass
    
    def find_prev(self):
        """寻找上一个"""
        pass
    
    def find_replace(self):
        """寻找替换"""
        find_text, ok1 = QInputDialog.getText(self, "寻找替换", "查找:")
        if ok1:
            replace_text, ok2 = QInputDialog.getText(self, "寻找替换", "替换:")
            if ok2:
                content = self.editor.toPlainText()
                new_content = content.replace(find_text, replace_text)
                self.save_state()
                self.editor.setPlainText(new_content)
                self.show_output(f"已替换: {find_text} -> {replace_text}")
    
    def find_all(self):
        """整体搜寻"""
        pass
    
    def show_options(self):
        """显示选项对话框"""
        dialog = QDialog(self)
        dialog.setWindowTitle("运行选项")
        dialog.resize(400, 350)
        
        layout = QVBoxLayout(dialog)
        
        # 格式化选项
        format_group = QGroupBox("格式化选项")
        format_layout = QVBoxLayout(format_group)
        
        auto_format_check = QCheckBox("自动格式化")
        auto_format_check.setChecked(self.auto_format)
        format_layout.addWidget(auto_format_check)
        
        no_format_check = QCheckBox("不格式化")
        no_format_check.setChecked(self.no_format)
        format_layout.addWidget(no_format_check)
        
        layout.addWidget(format_group)
        
        # 调试选项
        debug_group = QGroupBox("调试选项")
        debug_layout = QVBoxLayout(debug_group)
        
        debug_label = QLabel("调试级别:")
        debug_layout.addWidget(debug_label)
        
        debug_combo = QComboBox()
        debug_combo.addItems(["0", "1", "2", "3"])
        debug_combo.setCurrentText(self.debug_level)
        debug_layout.addWidget(debug_combo)
        
        layout.addWidget(debug_group)
        
        # 内存存储选项
        memory_group = QGroupBox("内存存储")
        memory_layout = QVBoxLayout(memory_group)
        
        memory_label = QLabel("存储模式:")
        memory_layout.addWidget(memory_label)
        
        memory_combo = QComboBox()
        memory_combo.addItems(["无", "无保留", "文本", "内存"])
        memory_combo.setCurrentText(self.memory_option)
        memory_layout.addWidget(memory_combo)
        
        layout.addWidget(memory_group)
        
        # 输出选项
        output_group = QGroupBox("输出选项")
        output_layout = QVBoxLayout(output_group)
        
        clear_output_check = QCheckBox("每次运行清屏")
        clear_output_check.setChecked(self.clear_output)
        output_layout.addWidget(clear_output_check)
        
        layout.addWidget(output_group)
        
        # 按钮
        button_layout = QHBoxLayout()
        ok_button = QPushButton("确定")
        cancel_button = QPushButton("取消")
        
        button_layout.addStretch()
        button_layout.addWidget(ok_button)
        button_layout.addWidget(cancel_button)
        layout.addLayout(button_layout)
        
        # 使用lambda来避免对话框被提前销毁
        ok_button.clicked.connect(lambda: self.save_options_and_accept(dialog, auto_format_check, no_format_check, debug_combo, memory_combo, clear_output_check))
        cancel_button.clicked.connect(dialog.reject)
        
        dialog.exec()
    
    def convert_chinese_to_english_text(self, text):
        """将中文符号转换为英文符号"""
        if not self.convert_chinese_to_english:
            return text
        
        # 中文符号到英文符号的映射
        chinese_to_english = {
            # 标点符号
            "，": ",",
            "。": ".",
            "；": ";",
            "：": ":",
            "！": "!",
            "？": "?",
            "（": "(",
            "）": ")",
            "【": "[",
            "】": "]",
            "｛": "{",
            "｝": "}",
            "《": "<",
            "》": ">",
            "、": ",",
            "「": "'",
            "」": "'",
            "『": "\"",
            "』": "\"",
            
            # 全角字符
            "　": " ",  # 全角空格
            "０": "0",
            "１": "1",
            "２": "2",
            "３": "3",
            "４": "4",
            "５": "5",
            "６": "6",
            "７": "7",
            "８": "8",
            "９": "9",
            "Ａ": "A",
            "Ｂ": "B",
            "Ｃ": "C",
            "Ｄ": "D",
            "Ｅ": "E",
            "Ｆ": "F",
            "Ｇ": "G",
            "Ｈ": "H",
            "Ｉ": "I",
            "Ｊ": "J",
            "Ｋ": "K",
            "Ｌ": "L",
            "Ｍ": "M",
            "Ｎ": "N",
            "Ｏ": "O",
            "Ｐ": "P",
            "Ｑ": "Q",
            "Ｒ": "R",
            "Ｓ": "S",
            "Ｔ": "T",
            "Ｕ": "U",
            "Ｖ": "V",
            "Ｗ": "W",
            "Ｘ": "X",
            "Ｙ": "Y",
            "Ｚ": "Z",
            "ａ": "a",
            "ｂ": "b",
            "ｃ": "c",
            "ｄ": "d",
            "ｅ": "e",
            "ｆ": "f",
            "ｇ": "g",
            "ｈ": "h",
            "ｉ": "i",
            "ｊ": "j",
            "ｋ": "k",
            "ｌ": "l",
            "ｍ": "m",
            "ｎ": "n",
            "ｏ": "o",
            "ｐ": "p",
            "ｑ": "q",
            "ｒ": "r",
            "ｓ": "s",
            "ｔ": "t",
            "ｕ": "u",
            "ｖ": "v",
            "ｗ": "w",
            "ｘ": "x",
            "ｙ": "y",
            "ｚ": "z"
        }
        
        # 如果需要排除字符串内容，先标记字符串位置
        if self.exclude_strings_from_conversion:
            converted_text = self.convert_with_string_exclusion(text, chinese_to_english)
        else:
            # 直接替换所有中文符号
            converted_text = text
            for chinese, english in chinese_to_english.items():
                converted_text = converted_text.replace(chinese, english)
        
        return converted_text
    
    def convert_with_string_exclusion(self, text, mappings):
        """在排除字符串内容的情况下进行转换"""
        result = []
        i = 0
        in_string = False
        string_char = None
        
        while i < len(text):
            char = text[i]
            
            # 检查是否进入或退出字符串
            if not in_string and char in '"\'':
                in_string = True
                string_char = char
                result.append(char)
            elif in_string and char == string_char:
                # 检查是否是转义字符
                if i > 0 and text[i-1] == '\\':
                    result.append(char)
                else:
                    in_string = False
                    string_char = None
                    result.append(char)
            elif in_string:
                # 在字符串中，不进行转换
                result.append(char)
            else:
                # 不在字符串中，进行转换
                converted = char
                for chinese, english in mappings.items():
                    if text.startswith(chinese, i):
                        converted = english
                        i += len(chinese) - 1
                        break
                result.append(converted)
            
            i += 1
        
        return ''.join(result)
    
    def save_options_and_accept(self, dialog, auto_format_check, no_format_check, debug_combo, memory_combo, clear_output_check):
        """保存选项并接受对话框"""
        self.auto_format = auto_format_check.isChecked()
        self.no_format = no_format_check.isChecked()
        self.debug_level = debug_combo.currentText()
        self.memory_option = memory_combo.currentText()
        self.clear_output = clear_output_check.isChecked()
        self.save_config()
        dialog.accept()
    
    def show_about(self):
        """显示现代化的Markdown帮助对话框"""
        try:
            with open("README.md", "r", encoding="utf-8") as f:
                readme_content = f.read()
            
            # 创建现代化的帮助对话框
            dialog = ModernHelpDialog(self, readme_content)
            dialog.setWindowTitle("CH+ 中文编辑器 - 帮助文档")
            dialog.resize(1000, 700)
            
            # 居中显示
            screen_geometry = QApplication.primaryScreen().availableGeometry()
            dialog_geometry = dialog.frameGeometry()
            dialog.move(
                (screen_geometry.width() - dialog_geometry.width()) // 2,
                (screen_geometry.height() - dialog_geometry.height()) // 2
            )
            
            dialog.exec()
        
        except Exception as e:
            QMessageBox.critical(self, "错误", f"无法读取README.md文件：{str(e)}")
    
    def show_settings(self):
        """显示设置对话框"""
        dialog = QDialog(self)
        dialog.setWindowTitle("设置")
        dialog.resize(800, 525)
        
        # 创建主布局
        main_layout = QVBoxLayout(dialog)
        main_layout.setContentsMargins(10, 10, 10, 10)
        main_layout.setSpacing(5)
        
        # 创建内容区域布局
        content_layout = QHBoxLayout()
        
        # 创建左侧栏目
        left_widget = QWidget()
        left_layout = QVBoxLayout(left_widget)
        left_widget.setFixedWidth(180)
        left_layout.setContentsMargins(10, 5, 10, 5)
        left_layout.setSpacing(0)
        
        # 创建左侧树状结构（带动态滚动条）
        left_scroll = QScrollArea()
        left_scroll.setWidgetResizable(True)
        left_scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        left_scroll.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
        
        left_content = QWidget()
        left_content_layout = QVBoxLayout(left_content)
        left_content_layout.setContentsMargins(0, 0, 0, 0)
        left_content_layout.setSpacing(0)
        left_content.setStyleSheet("background-color: #FFFFFF;")
        
        tree_widget = QTreeWidget()
        tree_widget.setHeaderHidden(True)
        tree_widget.setStyleSheet("""
            QTreeWidget {
                background-color: #FFFFFF;
                color: #000000;
                border: none;
            }
            QTreeWidget::item {
                padding: 2px;
            }
            QTreeWidget::item:selected {
                background-color: #0078D4;
                color: #FFFFFF;
            }
        """)
        
        # 创建根节点（只保留当前已有的功能）
        env_root = QTreeWidgetItem(tree_widget, ["环境"])
        editor_root = QTreeWidgetItem(tree_widget, ["编辑器"])
        lang_root = QTreeWidgetItem(tree_widget, ["语言"])
        
        # 创建子节点
        env_appearance = QTreeWidgetItem(env_root, ["外观"])
        editor_general = QTreeWidgetItem(editor_root, ["通用"])
        editor_format = QTreeWidgetItem(editor_root, ["格式"])
        editor_colors = QTreeWidgetItem(editor_root, ["颜色配置"])
        lang_custom = QTreeWidgetItem(lang_root, ["自定义关键字"])
        
        # 展开所有节点
        tree_widget.expandAll()
        
        left_content_layout.addWidget(tree_widget)
        left_content_layout.addStretch()
        left_scroll.setWidget(left_content)
        left_layout.addWidget(left_scroll)
        
        # 创建右侧内容区域（带动态滚动条）
        scroll_area = QScrollArea()
        scroll_area.setWidgetResizable(True)
        scroll_area.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
        scroll_area.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
        
        # 创建内容页面
        pages = {}
        
        # 环境-外观页面
        appearance_page = QWidget()
        appearance_layout = QVBoxLayout(appearance_page)
        appearance_layout.setSpacing(8)
        
        # 主题设置
        theme_group = QGroupBox("外观")
        theme_layout = QVBoxLayout(theme_group)
        theme_layout.setSpacing(6)
        
        theme_row = QHBoxLayout()
        theme_row.addWidget(QLabel("主题："))
        theme_combo = QComboBox()
        theme_combo.addItems(["浅色", "深色"])
        theme_combo.setCurrentText(self.settings["theme"])
        theme_row.addWidget(theme_combo)
        theme_row.addStretch()
        theme_layout.addLayout(theme_row)
        
        # 字体设置
        font_row = QHBoxLayout()
        font_row.addWidget(QLabel("字体："))
        font_combo = QComboBox()
        font_combo.addItems(["Microsoft YaHei UI", "Consolas", "Arial"])
        font_combo.setCurrentText(self.settings["font"])
        font_row.addWidget(font_combo)
        font_row.addStretch()
        theme_layout.addLayout(font_row)
        
        # 字体大小设置
        size_row = QHBoxLayout()
        size_row.addWidget(QLabel("大小："))
        size_spin = QSpinBox()
        size_spin.setRange(8, 24)
        size_spin.setValue(self.settings["font_size"])
        size_row.addWidget(size_spin)
        size_row.addStretch()
        theme_layout.addLayout(size_row)
        
        # 图标集设置
        icon_row = QHBoxLayout()
        icon_row.addWidget(QLabel("图标集："))
        icon_combo = QComboBox()
        icon_combo.addItems(["新外观"])
        icon_combo.setCurrentText(self.settings["icon_set"])
        icon_row.addWidget(icon_combo)
        icon_check = QCheckBox("使用自定义图标")
        icon_check.setChecked(self.settings["use_custom_icons"])
        icon_row.addWidget(icon_check)
        icon_row.addStretch()
        theme_layout.addLayout(icon_row)
        
        # 图标缩放设置
        icon_scale_row = QHBoxLayout()
        icon_scale_row.addWidget(QLabel("图标缩放："))
        icon_scale_spin = QDoubleSpinBox()
        icon_scale_spin.setRange(0.5, 2.0)
        icon_scale_spin.setSingleStep(0.1)
        icon_scale_spin.setValue(self.settings["icon_scale"])
        icon_scale_row.addWidget(icon_scale_spin)
        icon_scale_row.addStretch()
        theme_layout.addLayout(icon_scale_row)
        
        # 语言设置
        lang_row = QHBoxLayout()
        lang_row.addWidget(QLabel("语言："))
        lang_combo = QComboBox()
        lang_combo.addItems(["简体中文", "English"])
        lang_combo.setCurrentText(self.settings["language"])
        lang_row.addWidget(lang_combo)
        lang_note = QLabel("*需要重启之后生效")
        lang_note.setStyleSheet("color: #666666; font-size: 10px;")
        lang_row.addWidget(lang_note)
        lang_row.addStretch()
        theme_layout.addLayout(lang_row)
        
        # 鼠标滚轮设置
        wheel_check = QCheckBox("可通过转动鼠标滚轮来切换下拉框的当前选项")
        wheel_check.setChecked(self.settings["wheel_scroll"])
        theme_layout.addWidget(wheel_check)
        
        appearance_layout.addWidget(theme_group)
        appearance_layout.addStretch()
        pages["环境-外观"] = appearance_page
        
        # 编辑器-通用页面
        editor_general_page = QWidget()
        editor_general_layout = QVBoxLayout(editor_general_page)
        editor_general_layout.setSpacing(8)
        
        # 缩进设置
        indent_group = QGroupBox("缩进")
        indent_layout = QVBoxLayout(indent_group)
        indent_layout.setSpacing(4)
        
        indent_auto_check = QCheckBox("自动计算缩进")
        indent_auto_check.setChecked(self.settings["indent_auto"])
        indent_layout.addWidget(indent_auto_check)
        
        indent_space_check = QCheckBox("自动使用空格代替制表符(Tab)")
        indent_space_check.setChecked(self.settings["indent_use_spaces"])
        indent_layout.addWidget(indent_space_check)
        
        tab_width_row = QHBoxLayout()
        tab_width_row.addWidget(QLabel("制表符(Tab)宽度："))
        tab_width_spin = QSpinBox()
        tab_width_spin.setRange(1, 8)
        tab_width_spin.setValue(self.settings["tab_width"])
        tab_width_row.addWidget(tab_width_spin)
        tab_width_row.addStretch()
        indent_layout.addLayout(tab_width_row)
        
        indent_guide_check = QCheckBox("显示缩进提示线")
        indent_guide_check.setChecked(self.settings["indent_guide"])
        indent_layout.addWidget(indent_guide_check)
        
        indent_fill_check = QCheckBox("填充缩进区域")
        indent_fill_check.setChecked(self.settings["indent_fill"])
        indent_layout.addWidget(indent_fill_check)
        
        editor_general_layout.addWidget(indent_group)
        
        # 光标设置
        cursor_group = QGroupBox("光标")
        cursor_layout = QVBoxLayout(cursor_group)
        cursor_layout.setSpacing(4)
        
        cursor_home_check = QCheckBox("按下HOME键时，光标定位在本行的第一个非空格字符处")
        cursor_home_check.setChecked(self.settings["cursor_home_non_space"])
        cursor_layout.addWidget(cursor_home_check)
        
        cursor_end_check = QCheckBox("按下End键时，光标定位在本行的最后一个非空格字符处")
        cursor_end_check.setChecked(self.settings["cursor_end_non_space"])
        cursor_layout.addWidget(cursor_end_check)
        
        cursor_remember_check = QCheckBox("在上下移动光标时，记住起始时光标所在栏数")
        cursor_remember_check.setChecked(self.settings["cursor_remember_column"])
        cursor_layout.addWidget(cursor_remember_check)
        
        insert_cursor_row = QHBoxLayout()
        insert_cursor_row.addWidget(QLabel("插入状态下的光标："))
        insert_cursor_combo = QComboBox()
        insert_cursor_combo.addItems(["竖线", "块"])
        insert_cursor_combo.setCurrentText(self.settings["insert_cursor"])
        insert_cursor_row.addWidget(insert_cursor_combo)
        insert_cursor_row.addStretch()
        cursor_layout.addLayout(insert_cursor_row)
        
        overwrite_cursor_row = QHBoxLayout()
        overwrite_cursor_row.addWidget(QLabel("覆写状态下的光标："))
        overwrite_cursor_combo = QComboBox()
        overwrite_cursor_combo.addItems(["块", "竖线"])
        overwrite_cursor_combo.setCurrentText(self.settings["overwrite_cursor"])
        overwrite_cursor_row.addWidget(overwrite_cursor_combo)
        overwrite_cursor_row.addStretch()
        cursor_layout.addLayout(overwrite_cursor_row)
        
        cursor_color_check = QCheckBox("使用文字颜色作为光标颜色")
        cursor_color_check.setChecked(self.settings["cursor_use_text_color"])
        cursor_layout.addWidget(cursor_color_check)
        
        editor_general_layout.addWidget(cursor_group)
        
        # 高亮显示设置
        highlight_group = QGroupBox("高亮显示")
        highlight_layout = QVBoxLayout(highlight_group)
        highlight_layout.setSpacing(4)
        
        highlight_bracket_check = QCheckBox("高亮显示与光标处相匹配的括号")
        highlight_bracket_check.setChecked(self.settings["highlight_bracket"])
        highlight_layout.addWidget(highlight_bracket_check)
        
        highlight_word_check = QCheckBox("高亮显示光标所在的单词")
        highlight_word_check.setChecked(self.settings["highlight_word"])
        highlight_layout.addWidget(highlight_word_check)
        
        editor_general_layout.addWidget(highlight_group)
        
        # 中文转换设置
        chinese_convert_group = QGroupBox("中文转换")
        chinese_convert_layout = QVBoxLayout(chinese_convert_group)
        chinese_convert_layout.setSpacing(4)
        
        chinese_convert_check = QCheckBox("自动将中文字符转换为英文字符")
        chinese_convert_check.setChecked(self.settings.get("chinese_convert_enabled", False))
        chinese_convert_layout.addWidget(chinese_convert_check)
        
        chinese_exclude_string_check = QCheckBox("排除字符串内容（不转换引号内的内容）")
        chinese_exclude_string_check.setChecked(self.settings.get("chinese_exclude_strings", True))
        chinese_convert_layout.addWidget(chinese_exclude_string_check)
        
        editor_general_layout.addWidget(chinese_convert_group)
        
        # 滚动条设置
        scroll_group = QGroupBox("滚动条")
        scroll_layout = QVBoxLayout(scroll_group)
        scroll_layout.setSpacing(4)
        
        scroll_hide_check = QCheckBox("自动隐藏滚动条")
        scroll_hide_check.setChecked(self.settings["scroll_auto_hide"])
        scroll_layout.addWidget(scroll_hide_check)
        
        scroll_left_check = QCheckBox("可以将每行末尾字符滚动到编辑器最左侧")
        scroll_left_check.setChecked(self.settings["scroll_end_to_left"])
        scroll_layout.addWidget(scroll_left_check)
        
        scroll_top_check = QCheckBox("可以将最后一行滚动到编辑器最上方")
        scroll_top_check.setChecked(self.settings["scroll_last_to_top"])
        scroll_layout.addWidget(scroll_top_check)
        
        scroll_half_check = QCheckBox("翻页键只滚动半页")
        scroll_half_check.setChecked(self.settings["scroll_half_page"])
        scroll_layout.addWidget(scroll_half_check)
        
        scroll_speed_row = QHBoxLayout()
        scroll_speed_row.addWidget(QLabel("鼠标滚轮卷速（行）："))
        scroll_speed_spin = QSpinBox()
        scroll_speed_spin.setRange(1, 10)
        scroll_speed_spin.setValue(self.settings["scroll_speed"])
        scroll_speed_row.addWidget(scroll_speed_spin)
        scroll_speed_row.addStretch()
        scroll_layout.addLayout(scroll_speed_row)
        
        scroll_drag_row = QHBoxLayout()
        scroll_drag_row.addWidget(QLabel("鼠标选择/拖拽卷轴速度："))
        scroll_drag_spin = QSpinBox()
        scroll_drag_spin.setRange(1, 20)
        scroll_drag_spin.setValue(self.settings["scroll_drag_speed"])
        scroll_drag_row.addWidget(scroll_drag_spin)
        scroll_drag_row.addStretch()
        scroll_layout.addLayout(scroll_drag_row)
        
        scroll_edge_check = QCheckBox("显示右边边缘线")
        scroll_edge_check.setChecked(self.settings["scroll_edge"])
        scroll_layout.addWidget(scroll_edge_check)
        
        scroll_edge_width_row = QHBoxLayout()
        scroll_edge_width_row.addWidget(QLabel("右边缘宽度："))
        scroll_edge_width_spin = QSpinBox()
        scroll_edge_width_spin.setRange(40, 120)
        scroll_edge_width_spin.setValue(self.settings["scroll_edge_width"])
        scroll_edge_width_row.addWidget(scroll_edge_width_spin)
        scroll_edge_width_row.addStretch()
        scroll_layout.addLayout(scroll_edge_width_row)
        
        scroll_edge_color_row = QHBoxLayout()
        scroll_edge_color_row.addWidget(QLabel("右边缘颜色："))
        scroll_edge_color_edit = QLineEdit(self.settings["scroll_edge_color"])
        scroll_edge_color_edit.setMaximumWidth(100)
        scroll_edge_color_row.addWidget(scroll_edge_color_edit)
        
        scroll_edge_color_btn = QPushButton("选择")
        scroll_edge_color_btn.setMaximumWidth(60)
        scroll_edge_color_row.addWidget(scroll_edge_color_btn)
        
        def pick_scroll_edge_color():
            current_color = QColor(scroll_edge_color_edit.text())
            color = QColorDialog.getColor(current_color, self)
            if color.isValid():
                scroll_edge_color_edit.setText(color.name())
        
        scroll_edge_color_btn.clicked.connect(pick_scroll_edge_color)
        
        scroll_edge_color_row.addStretch()
        scroll_layout.addLayout(scroll_edge_color_row)
        
        editor_general_layout.addWidget(scroll_group)
        editor_general_layout.addStretch()
        pages["编辑器-通用"] = editor_general_page
        
        # 编辑器-格式页面
        editor_format_page = QWidget()
        editor_format_layout = QVBoxLayout(editor_format_page)
        editor_format_layout.setSpacing(8)
        
        # 格式设置
        format_group = QGroupBox("格式设置")
        format_layout = QGridLayout(format_group)
        format_layout.setHorizontalSpacing(10)
        format_layout.setVerticalSpacing(6)
        
        format_space_check = QCheckBox("在代码段之间加入空行")
        format_space_check.setChecked(self.settings["format_space_between_sections"])
        format_layout.addWidget(format_space_check, 0, 0)
        
        format_all_space_check = QCheckBox("在所有代码段之间加入空行")
        format_all_space_check.setChecked(self.settings["format_space_between_all_sections"])
        format_layout.addWidget(format_all_space_check, 0, 1)
        
        format_remove_check = QCheckBox("删除超过指定数量的多余空行")
        format_remove_check.setChecked(self.settings["format_remove_extra_lines"])
        format_layout.addWidget(format_remove_check, 1, 0)
        
        format_remove_spin = QSpinBox()
        format_remove_spin.setRange(1, 5)
        format_remove_spin.setValue(self.settings["format_remove_extra_lines_count"])
        format_layout.addWidget(format_remove_spin, 1, 1)
        
        format_comma_check = QCheckBox("在逗号后插入空格")
        format_comma_check.setChecked(self.settings["format_space_after_comma"])
        format_layout.addWidget(format_comma_check, 2, 0)
        
        format_op_check = QCheckBox("在运算符周围插入空格")
        format_op_check.setChecked(self.settings["format_space_around_operator"])
        format_layout.addWidget(format_op_check, 2, 1)
        
        format_paren_inner_check = QCheckBox("在括号内侧加入空格")
        format_paren_inner_check.setChecked(self.settings["format_space_inside_paren"])
        format_layout.addWidget(format_paren_inner_check, 3, 0)
        
        format_paren_both_check = QCheckBox("在括号两侧加入空格")
        format_paren_both_check.setChecked(self.settings["format_space_both_paren"])
        format_layout.addWidget(format_paren_both_check, 3, 1)
        
        format_paren_outer_check = QCheckBox("在括号外侧加入空格")
        format_paren_outer_check.setChecked(self.settings["format_space_outside_paren"])
        format_layout.addWidget(format_paren_outer_check, 4, 0)
        
        format_paren_multi_check = QCheckBox("在多层嵌套括号的最外侧加入空格")
        format_paren_multi_check.setChecked(self.settings["format_space_multi_paren"])
        format_layout.addWidget(format_paren_multi_check, 4, 1)
        
        format_stmt_check = QCheckBox("在语句和括号间插入空格('if','for'...)")
        format_stmt_check.setChecked(self.settings["format_space_after_stmt"])
        format_layout.addWidget(format_stmt_check, 5, 0)
        
        format_remove_space_check = QCheckBox("删除多余的空格")
        format_remove_space_check.setChecked(self.settings["format_remove_extra_space"])
        format_layout.addWidget(format_remove_space_check, 5, 1)
        
        editor_format_layout.addWidget(format_group)
        editor_format_layout.addStretch()
        pages["编辑器-格式"] = editor_format_page
        
        # 语言-自定义关键字页面
        lang_custom_page = QWidget()
        lang_custom_layout = QVBoxLayout(lang_custom_page)
        lang_custom_layout.setSpacing(8)
        
        # 自定义关键字设置
        custom_group = QGroupBox("自定义关键字")
        custom_layout = QVBoxLayout(custom_group)
        custom_layout.setSpacing(6)
        
        custom_enable_check = QCheckBox("启用自定义类型关键字")
        custom_enable_check.setChecked(self.settings["custom_keywords_enabled"])
        custom_layout.addWidget(custom_enable_check)
        
        custom_toolbar = QHBoxLayout()
        custom_add_btn = QPushButton("+")
        custom_add_btn.setFixedWidth(30)
        custom_remove_btn = QPushButton("-")
        custom_remove_btn.setFixedWidth(30)
        custom_import_btn = QPushButton("导入")
        custom_toolbar.addWidget(custom_add_btn)
        custom_toolbar.addWidget(custom_remove_btn)
        custom_toolbar.addWidget(custom_import_btn)
        custom_toolbar.addStretch()
        custom_layout.addLayout(custom_toolbar)
        
        custom_edit = QTextEdit()
        custom_edit.setMinimumHeight(200)
        # 一行一个关键字，用逗号分隔的改为换行分隔
        keywords_text = "\n".join(self.settings["custom_keywords"])
        custom_edit.setPlainText(keywords_text)
        custom_layout.addWidget(custom_edit)
        
        custom_note = QLabel("注意：自定义关键字对语法检查器无效。")
        custom_note.setStyleSheet("color: #666666; font-size: 10px;")
        custom_layout.addWidget(custom_note)
        
        lang_custom_layout.addWidget(custom_group)
        lang_custom_layout.addStretch()
        pages["语言-自定义关键字"] = lang_custom_page
        
        # 编辑器-颜色配置页面
        editor_colors_page = QWidget()
        editor_colors_layout = QVBoxLayout(editor_colors_page)
        editor_colors_layout.setSpacing(8)
        
        # 创建滚动区域用于颜色配置（动态滑动条）
        colors_scroll = QScrollArea()
        colors_scroll.setWidgetResizable(True)
        colors_scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
        colors_scroll.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
        
        # 颜色配置设置
        colors_widget = QWidget()
        colors_layout = QVBoxLayout(colors_widget)
        colors_layout.setSpacing(8)
        
        colors_group = QGroupBox("颜色配置")
        colors_group_layout = QVBoxLayout(colors_group)
        colors_group_layout.setSpacing(6)
        
        # 创建颜色配置行（单列显示）
        color_configs = [
            ("关键字颜色", "color_keyword", "#0000FF"),
            ("注释颜色", "color_comment", "#808080"),
            ("字符串颜色", "color_string", "#FF0000"),
            ("数字颜色", "color_number", "#FF8C00"),
            ("运算符颜色", "color_operator", "#000000"),
            ("标识符颜色", "color_identifier", "#000000"),
            ("背景颜色", "color_background", "#FFFFFF"),
            ("行号颜色", "color_line_number", "#808080"),
            ("当前行颜色", "color_current_line", "#F0F8FF"),
            ("选择颜色", "color_selection", "#ADD8E6"),
            ("光标颜色", "color_cursor", "#000000")
        ]
        
        self.color_edits = {}
        for label_text, setting_key, default_color in color_configs:
            # 创建单行布局
            color_row = QHBoxLayout()
            color_row.setSpacing(10)
            
            # 创建标签
            label = QLabel(label_text)
            label.setMinimumWidth(120)
            color_row.addWidget(label)
            
            # 创建颜色输入框
            color_edit = QLineEdit()
            color_edit.setText(self.settings[setting_key])
            color_edit.setPlaceholderText(default_color)
            color_edit.setMaximumWidth(100)
            color_row.addWidget(color_edit)
            
            # 创建颜色选择按钮
            color_btn = QPushButton("选择")
            color_btn.setMaximumWidth(60)
            color_row.addWidget(color_btn)
            
            # 添加弹性空间
            color_row.addStretch()
            
            # 存储颜色编辑框
            self.color_edits[setting_key] = color_edit
            
            # 连接颜色选择按钮
            def create_color_picker(setting_key=setting_key):
                def pick_color():
                    current_color = QColor(self.color_edits[setting_key].text())
                    color = QColorDialog.getColor(current_color, self)
                    if color.isValid():
                        self.color_edits[setting_key].setText(color.name())
                return pick_color
            
            color_btn.clicked.connect(create_color_picker())
            
            # 添加到颜色组布局
            colors_group_layout.addLayout(color_row)
        
        colors_layout.addWidget(colors_group)
        colors_layout.addStretch()
        
        # 设置滚动区域的内容
        colors_scroll.setWidget(colors_widget)
        
        # 动态调整滑动条显示和滚动功能（只在需要时启用）
        def adjust_scrollbar():
            # 获取内容高度和滚动区域高度
            content_height = colors_widget.sizeHint().height()
            scroll_height = colors_scroll.viewport().height()
            
            # 如果内容高度小于滚动区域高度，隐藏垂直滑动条并禁用滚动
            if content_height <= scroll_height:
                colors_scroll.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
                # 禁用垂直滚动
                colors_scroll.verticalScrollBar().setEnabled(False)
            else:
                colors_scroll.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
                # 启用垂直滚动
                colors_scroll.verticalScrollBar().setEnabled(True)
            
            # 获取内容宽度和滚动区域宽度
            content_width = colors_widget.sizeHint().width()
            scroll_width = colors_scroll.viewport().width()
            
            # 如果内容宽度小于滚动区域宽度，隐藏水平滑动条并禁用滚动
            if content_width <= scroll_width:
                colors_scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
                # 禁用水平滚动
                colors_scroll.horizontalScrollBar().setEnabled(False)
            else:
                colors_scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
                # 启用水平滚动
                colors_scroll.horizontalScrollBar().setEnabled(True)
        
        # 在对话框显示后调整滑动条
        QTimer.singleShot(0, adjust_scrollbar)
        
        # 监听窗口大小变化
        def dialog_resize_event(event):
            super(QDialog, dialog).resizeEvent(event)
            adjust_scrollbar()
        
        dialog.resizeEvent = dialog_resize_event
        
        # 添加到主布局
        editor_colors_layout.addWidget(colors_scroll)
        pages["编辑器-颜色配置"] = editor_colors_page
        
        # 创建当前显示的页面
        current_page = QStackedWidget()
        for page in pages.values():
            current_page.addWidget(page)
        
        # 将页面堆栈设置为滚动区域的widget
        scroll_area.setWidget(current_page)
        
        # 默认显示第一个页面
        current_page.setCurrentWidget(appearance_page)
        
        # 连接树状结构的点击事件
        def on_tree_item_clicked(item, column):
            parent = item.parent()
            if parent:
                # 对于有父节点的项，使用 "父节点-子节点" 作为键
                key = f"{parent.text(0)}-{item.text(0)}"
                if key in pages:
                    current_page.setCurrentWidget(pages[key])
            else:
                # 如果根节点没有对应的页面，显示第一个子节点的页面
                if item.childCount() > 0:
                    first_child = item.child(0)
                    key = f"{item.text(0)}-{first_child.text(0)}"
                    if key in pages:
                        current_page.setCurrentWidget(pages[key])
        
        tree_widget.itemClicked.connect(on_tree_item_clicked)
        
        # 添加到布局
        content_layout.addWidget(left_widget)
        content_layout.addWidget(scroll_area)
        content_layout.setStretch(0, 0)
        content_layout.setStretch(1, 1)
        main_layout.addLayout(content_layout)
        
        # 动态调整滑动条显示和滚动功能（只在需要时启用）
        def adjust_scroll_areas():
            # 动态调整左侧滚动条和滚动功能
            left_content_height = left_content.sizeHint().height()
            left_scroll_height = left_scroll.viewport().height()
            
            if left_content_height <= left_scroll_height:
                left_scroll.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
                # 禁用垂直滚动
                left_scroll.verticalScrollBar().setEnabled(False)
            else:
                left_scroll.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
                # 启用垂直滚动
                left_scroll.verticalScrollBar().setEnabled(True)
            
            # 动态调整右侧滚动条和滚动功能
            current_page_height = current_page.currentWidget().sizeHint().height()
            scroll_area_height = scroll_area.viewport().height()
            
            if current_page_height <= scroll_area_height:
                scroll_area.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
                # 禁用垂直滚动
                scroll_area.verticalScrollBar().setEnabled(False)
            else:
                scroll_area.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
                # 启用垂直滚动
                scroll_area.verticalScrollBar().setEnabled(True)
            
            # 动态调整右侧水平滚动条和滚动功能
            current_page_width = current_page.currentWidget().sizeHint().width()
            scroll_area_width = scroll_area.viewport().width()
            
            if current_page_width <= scroll_area_width:
                scroll_area.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
                # 禁用水平滚动
                scroll_area.horizontalScrollBar().setEnabled(False)
            else:
                scroll_area.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
                # 启用水平滚动
                scroll_area.horizontalScrollBar().setEnabled(True)
        
        # 在对话框显示后调整高度和滑动条
        QTimer.singleShot(0, adjust_scroll_areas)
        
        # 监听页面切换事件
        def on_page_changed(index):
            adjust_scroll_areas()
        
        current_page.currentChanged.connect(on_page_changed)
        
        # 监听窗口大小变化
        def dialog_resize_event_main(event):
            super(QDialog, dialog).resizeEvent(event)
            adjust_scroll_areas()
        
        dialog.resizeEvent = dialog_resize_event_main
        
        # 创建按钮布局
        button_widget = QWidget()
        button_layout = QHBoxLayout(button_widget)
        button_widget.setFixedHeight(50)
        button_layout.setContentsMargins(10, 5, 10, 5)
        
        ok_button = QPushButton("确定")
        ok_button.setMinimumSize(100, 35)
        ok_button.setMaximumSize(100, 35)
        
        cancel_button = QPushButton("取消")
        cancel_button.setMinimumSize(100, 35)
        cancel_button.setMaximumSize(100, 35)
        
        apply_button = QPushButton("应用")
        apply_button.setMinimumSize(100, 35)
        apply_button.setMaximumSize(100, 35)
        
        button_layout.addStretch()
        button_layout.addWidget(ok_button)
        button_layout.addWidget(cancel_button)
        button_layout.addWidget(apply_button)
        
        # 添加按钮布局到主布局
        main_layout.addWidget(button_widget)
        
        # 应用设置
        def apply_settings():
            # 保存环境-外观设置
            self.settings["theme"] = theme_combo.currentText()
            self.settings["font"] = font_combo.currentText()
            self.settings["font_size"] = size_spin.value()
            self.settings["icon_set"] = icon_combo.currentText()
            self.settings["use_custom_icons"] = icon_check.isChecked()
            self.settings["icon_scale"] = icon_scale_spin.value()
            self.settings["language"] = lang_combo.currentText()
            self.settings["wheel_scroll"] = wheel_check.isChecked()
            
            # 保存编辑器-通用设置
            self.settings["indent_auto"] = indent_auto_check.isChecked()
            self.settings["indent_use_spaces"] = indent_space_check.isChecked()
            self.settings["tab_width"] = tab_width_spin.value()
            self.settings["indent_guide"] = indent_guide_check.isChecked()
            self.settings["indent_fill"] = indent_fill_check.isChecked()
            self.settings["cursor_home_non_space"] = cursor_home_check.isChecked()
            self.settings["cursor_end_non_space"] = cursor_end_check.isChecked()
            self.settings["cursor_remember_column"] = cursor_remember_check.isChecked()
            self.settings["insert_cursor"] = insert_cursor_combo.currentText()
            self.settings["overwrite_cursor"] = overwrite_cursor_combo.currentText()
            self.settings["cursor_use_text_color"] = cursor_color_check.isChecked()
            self.settings["highlight_bracket"] = highlight_bracket_check.isChecked()
            self.settings["highlight_word"] = highlight_word_check.isChecked()
            self.settings["scroll_auto_hide"] = scroll_hide_check.isChecked()
            self.settings["scroll_end_to_left"] = scroll_left_check.isChecked()
            self.settings["scroll_last_to_top"] = scroll_top_check.isChecked()
            self.settings["scroll_half_page"] = scroll_half_check.isChecked()
            self.settings["scroll_speed"] = scroll_speed_spin.value()
            self.settings["scroll_drag_speed"] = scroll_drag_spin.value()
            self.settings["scroll_edge"] = scroll_edge_check.isChecked()
            self.settings["scroll_edge_width"] = scroll_edge_width_spin.value()
            self.settings["scroll_edge_color"] = scroll_edge_color_edit.text()
            
            # 保存中文转换设置
            self.settings["chinese_convert_enabled"] = chinese_convert_check.isChecked()
            self.settings["chinese_exclude_strings"] = chinese_exclude_string_check.isChecked()
            
            # 保存编辑器-格式设置
            self.settings["format_space_between_sections"] = format_space_check.isChecked()
            self.settings["format_space_between_all_sections"] = format_all_space_check.isChecked()
            self.settings["format_remove_extra_lines"] = format_remove_check.isChecked()
            self.settings["format_remove_extra_lines_count"] = format_remove_spin.value()
            self.settings["format_space_after_comma"] = format_comma_check.isChecked()
            self.settings["format_space_around_operator"] = format_op_check.isChecked()
            self.settings["format_space_inside_paren"] = format_paren_inner_check.isChecked()
            self.settings["format_space_both_paren"] = format_paren_both_check.isChecked()
            self.settings["format_space_outside_paren"] = format_paren_outer_check.isChecked()
            self.settings["format_space_multi_paren"] = format_paren_multi_check.isChecked()
            self.settings["format_space_after_stmt"] = format_stmt_check.isChecked()
            self.settings["format_remove_extra_space"] = format_remove_space_check.isChecked()
            
            # 保存语言-自定义关键字设置
            self.settings["custom_keywords_enabled"] = custom_enable_check.isChecked()
            keywords_text = custom_edit.toPlainText()
            if keywords_text:
                # 一行一个关键字，按换行符分割
                self.settings["custom_keywords"] = [k.strip() for k in keywords_text.split("\n") if k.strip()]
            else:
                self.settings["custom_keywords"] = []
            
            # 保存颜色配置设置
            for setting_key, color_edit in self.color_edits.items():
                color_text = color_edit.text().strip()
                if color_text and QColor(color_text).isValid():
                    self.settings[setting_key] = color_text
            
            # 保存设置
            self.save_config()
            
            # 应用设置到UI
            self.apply_settings()
        
        ok_button.clicked.connect(lambda: (apply_settings(), dialog.accept()))
        cancel_button.clicked.connect(dialog.reject)
        apply_button.clicked.connect(apply_settings)
        
        dialog.exec()
    
    def load_config(self):
        """加载配置文件"""
        config = configparser.ConfigParser()
        
        if os.path.exists(self.config_file):
            try:
                config.read(self.config_file, encoding="utf-8")
                
                # 加载运行选项
                if "RunOptions" in config:
                    self.auto_format = config.getboolean("RunOptions", "auto_format", fallback=False)
                    self.no_format = config.getboolean("RunOptions", "no_format", fallback=False)
                    self.debug_level = config.get("RunOptions", "debug_level", fallback="0")
                    self.memory_option = config.get("RunOptions", "memory_option", fallback="无")
                    self.clear_output = config.getboolean("RunOptions", "clear_output", fallback=False)
                    self.convert_chinese_to_english = config.getboolean("RunOptions", "convert_chinese_to_english", fallback=False)
                
                # 加载设置选项
                if "Settings" in config:
                    for key, value in self.settings.items():
                        if key in config["Settings"]:
                            if isinstance(value, bool):
                                self.settings[key] = config.getboolean("Settings", key, fallback=value)
                            elif isinstance(value, int):
                                self.settings[key] = config.getint("Settings", key, fallback=value)
                            elif isinstance(value, float):
                                self.settings[key] = config.getfloat("Settings", key, fallback=value)
                            else:
                                self.settings[key] = config.get("Settings", key, fallback=value)
                    
                    # 从设置中加载中文转换选项
                    self.convert_chinese_to_english = config.getboolean("Settings", "chinese_convert_enabled", fallback=self.convert_chinese_to_english)
                    self.exclude_strings_from_conversion = config.getboolean("Settings", "chinese_exclude_strings", fallback=True)
                
                # 加载关键字设置
                if "Keywords" in config:
                    keywords_str = config.get("Keywords", "list", fallback="")
                    if keywords_str:
                        keywords = [k.strip() for k in keywords_str.split(",") if k.strip()]
                        self.highlighter.update_keywords(keywords)
                
                # 加载颜色设置
                if "Colors" in config:
                    # 加载所有颜色设置
                    color_keys = ["keyword", "comment", "string", "number", "operator", "identifier", 
                                 "background", "line_number", "current_line", "selection", "cursor"]
                    for key in color_keys:
                        color_str = config.get("Colors", key, fallback=self.settings[f"color_{key}"])
                        self.settings[f"color_{key}"] = color_str
                    
                    # 应用颜色设置到高亮器
                    if hasattr(self, 'highlighter'):
                        self.highlighter.update_colors(self.settings)
                
                # 加载设置选项
                if "Settings" in config:
                    for key, value in self.settings.items():
                        if key in config["Settings"]:
                            if isinstance(value, bool):
                                self.settings[key] = config.getboolean("Settings", key, fallback=value)
                            elif isinstance(value, int):
                                self.settings[key] = config.getint("Settings", key, fallback=value)
                            elif isinstance(value, float):
                                self.settings[key] = config.getfloat("Settings", key, fallback=value)
                            else:
                                self.settings[key] = config.get("Settings", key, fallback=value)
                
            except Exception as e:
                print(f"加载配置文件失败：{str(e)}")
    
    def apply_settings(self):
        """应用当前设置到UI组件"""
        # 应用字体设置
        font = QFont(self.settings["font"], self.settings["font_size"])
        self.editor.setFont(font)
        
        # 应用主题设置
        if self.settings["theme"] == "深色":
            self.apply_dark_theme()
        else:
            self.apply_light_theme()
        
        # 应用编辑器设置
        self.apply_editor_settings()
        
        # 应用颜色设置
        self.apply_color_settings()
        
        # 应用中文转换设置
        self.convert_chinese_to_english = self.settings.get("chinese_convert_enabled", False)
        self.exclude_strings_from_conversion = self.settings.get("chinese_exclude_strings", True)
    
    def apply_dark_theme(self):
        """应用深色主题"""
        dark_style = """
            QMainWindow {
                background-color: #2D2D30;
                color: #FFFFFF;
            }
            QPlainTextEdit {
                background-color: #1E1E1E;
                color: #D4D4D4;
                selection-background-color: #264F78;
            }
            QTreeWidget {
                background-color: #252526;
                color: #CCCCCC;
                border: none;
            }
            QMenuBar {
                background-color: #2D2D30;
                color: #FFFFFF;
            }
            QMenu {
                background-color: #2D2D30;
                color: #FFFFFF;
            }
        """
        self.setStyleSheet(dark_style)
    
    def apply_light_theme(self):
        """应用浅色主题"""
        light_style = """
            QMainWindow {
                background-color: #FFFFFF;
                color: #000000;
            }
            QPlainTextEdit {
                background-color: #FFFFFF;
                color: #000000;
                selection-background-color: #ADD8E6;
            }
            QTreeWidget {
                background-color: #F5F5F5;
                color: #000000;
                border: none;
            }
            QMenuBar {
                background-color: #F0F0F0;
                color: #000000;
            }
            QMenu {
                background-color: #FFFFFF;
                color: #000000;
            }
        """
        self.setStyleSheet(light_style)
    
    def apply_editor_settings(self):
        """应用编辑器相关设置"""
        # 设置Tab宽度
        self.editor.setTabStopDistance(self.settings["tab_width"] * self.editor.fontMetrics().horizontalAdvance(' '))
        
        # 设置光标样式
        if self.settings["cursor_use_text_color"]:
            # 使用QPalette设置光标颜色（替代不支持的caret-color CSS属性）
            palette = self.editor.palette()
            palette.setColor(QPalette.ColorRole.Text, QColor(self.settings["color_cursor"]))
            self.editor.setPalette(palette)
        
        # 设置当前行高亮
        if hasattr(self.editor, 'highlight_current_line'):
            self.editor.highlight_current_line()
    
    def apply_color_settings(self):
        """应用颜色设置到语法高亮器"""
        if hasattr(self, 'highlighter'):
            # 更新语法高亮器颜色
            self.highlighter.update_colors(self.settings)
    
    def save_config(self):
        """保存配置文件"""
        config = configparser.ConfigParser()
        
        # 保存运行选项
        config["RunOptions"] = {
            "auto_format": str(self.auto_format),
            "no_format": str(self.no_format),
            "debug_level": self.debug_level,
            "memory_option": self.memory_option,
            "clear_output": str(self.clear_output),
            "convert_chinese_to_english": str(self.convert_chinese_to_english),
            "exclude_strings_from_conversion": str(self.exclude_strings_from_conversion)
        }
        
        # 保存关键字设置
        config["Keywords"] = {
            "list": ",".join(self.highlighter.keywords)
        }
        
        # 保存颜色设置
        config["Colors"] = {
            "keyword": self.settings["color_keyword"],
            "comment": self.settings["color_comment"],
            "string": self.settings["color_string"],
            "number": self.settings["color_number"],
            "operator": self.settings["color_operator"],
            "identifier": self.settings["color_identifier"],
            "background": self.settings["color_background"],
            "line_number": self.settings["color_line_number"],
            "current_line": self.settings["color_current_line"],
            "selection": self.settings["color_selection"],
            "cursor": self.settings["color_cursor"]
        }
        
        # 保存设置选项
        config["Settings"] = {}
        for key, value in self.settings.items():
            config["Settings"][key] = str(value)
        
        try:
            with open(self.config_file, "w", encoding="utf-8") as f:
                config.write(f)
        except Exception as e:
            print(f"保存配置文件失败：{str(e)}")
    
    def restore_last_file(self):
        """恢复上一次打开的文件"""
        config = configparser.ConfigParser()
        
        if os.path.exists(self.config_file):
            try:
                config.read(self.config_file, encoding="utf-8")
                
                if "LastFile" in config:
                    last_file = config.get("LastFile", "path", fallback="")
                    if last_file and os.path.exists(last_file):
                        self.current_file = last_file
                        try:
                            with open(last_file, "r", encoding="utf-8") as f:
                                content = f.read()
                            self.editor.setPlainText(content)
                            # 设置光标到文档开始
                            cursor = self.editor.textCursor()
                            cursor.movePosition(QTextCursor.MoveOperation.Start)
                            self.editor.setTextCursor(cursor)
                            self.undo_stack.clear()
                            self.redo_stack.clear()
                            self.save_state()
                            self.show_output(f"已恢复上次打开的文件: {last_file}")
                        except Exception as e:
                            self.show_output(f"无法恢复文件：{str(e)}")
            except Exception as e:
                print(f"恢复上次文件失败：{str(e)}")
    
    def save_last_file(self):
        """保存当前打开的文件路径"""
        config = configparser.ConfigParser()
        
        if os.path.exists(self.config_file):
            config.read(self.config_file, encoding="utf-8")
        
        config["LastFile"] = {
            "path": self.current_file
        }
        
        try:
            with open(self.config_file, "w", encoding="utf-8") as f:
                config.write(f)
        except Exception as e:
            print(f"保存上次文件路径失败：{str(e)}")
    
    def show_output(self, text):
        """显示输出信息"""
        self.output.appendPlainText(text)
        self.output.ensureCursorVisible()
    
    def closeEvent(self, event):
        """窗口关闭事件"""
        self.save_config()
        self.save_last_file()
        event.accept()
    
    def show_keyword_documentation(self, keyword):
        """显示关键字文档"""
        if keyword in self.highlighter.keyword_documentation:
            documentation = self.highlighter.keyword_documentation[keyword]
            
            # 创建文档窗口
            doc_window = QDialog(self)
            doc_window.setWindowTitle(f"{keyword} - 文档")
            doc_window.resize(600, 400)
            
            # 创建布局
            layout = QVBoxLayout(doc_window)
            
            # 创建文本浏览器显示文档
            text_browser = QTextBrowser()
            text_browser.setPlainText(documentation)
            text_browser.setReadOnly(True)
            text_browser.setFont(QFont("Consolas", 10))
            
            # 添加到布局
            layout.addWidget(text_browser)
            
            # 显示窗口
            doc_window.exec()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    ide = CHPlusIDE()
    ide.show()
    sys.exit(app.exec())