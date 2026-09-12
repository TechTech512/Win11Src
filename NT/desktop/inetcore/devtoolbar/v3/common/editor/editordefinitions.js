//
//! Copyright (C) Microsoft. All rights reserved.
//
/// <reference path="../../../../internalapis/bptoob/inc/1.00/Plugin.d.ts" />
// This file is created partly by copying the .d.ts file at http://monacotools/doc#Editor_TypeScript_API.
// That file has some mistakes and omissions, so this file is also edited by hand as needed.
// Some methods were commented out to introduce cut-off points
// in order to not bring in our entire TypeScript universe
// Define this as a separate module so it doesn't conflict with the actual Monaco module
var MonacoDefinitions;
(function (MonacoDefinitions) {
    "use strict";
    var MouseTargetType = (function () {
        function MouseTargetType() {
        }
        MouseTargetType.UNKNOWN = 0;
        MouseTargetType.TEXTAREA = 1;
        MouseTargetType.GUTTER_GLYPH_MARGIN = 2;
        MouseTargetType.GUTTER_LINE_NUMBERS = 3;
        MouseTargetType.GUTTER_LINE_DECORATIONS = 4;
        MouseTargetType.GUTTER_VIEW_ZONE = 5;
        MouseTargetType.CONTENT_TEXT = 6;
        MouseTargetType.CONTENT_EMPTY = 7;
        MouseTargetType.CONTENT_VIEW_ZONE = 8;
        MouseTargetType.CONTENT_WIDGET = 9;
        MouseTargetType.OVERVIEW_RULER = 10;
        MouseTargetType.SCROLLBAR = 11;
        MouseTargetType.OVERLAY_WIDGET = 12;
        return MouseTargetType;
    })();
    MonacoDefinitions.MouseTargetType = MouseTargetType;
    var EndOfLinePreference = (function () {
        function EndOfLinePreference() {
        }
        EndOfLinePreference.TextDefined = 0;
        EndOfLinePreference.LF = 1;
        EndOfLinePreference.CRLF = 2;
        return EndOfLinePreference;
    })();
    MonacoDefinitions.EndOfLinePreference = EndOfLinePreference;
    var EventType = (function () {
        function EventType() {
        }
        EventType.ModelContentChanged = 'contentChanged';
        EventType.MouseMove = 'mousemove';
        return EventType;
    })();
    MonacoDefinitions.EventType = EventType;
})(MonacoDefinitions || (MonacoDefinitions = {}));
